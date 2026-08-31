#include "CTile.h"

#include <Windows.h>
#include "../Log/CLog.h"
#include "../CPlayer.h"
#include "../MainWorld/CMainWorld.h"

void CTile::TileJobRun()
{
	// 매 Tick마다 vector를 다시 할당하지 않도록 Tile 전용 작업 버퍼를 재사용한다.
	m_vecJobBuffer.clear();
	m_queue.PopVector(m_vecJobBuffer);
	for (st_TileJob& job : m_vecJobBuffer)
	{
		CEntity* pEntity = job.pEntity;
		if (pEntity == nullptr)
			continue;

		switch (job.type)
		{
		case ETILE_JOB_TYPE::WRONG_ENTITY_REMOVE:
			RemovePlayer(job.pEntity);
			break;
		default:
			break;
		}

		// Enqueue 에서 획득한 작업 참조를 반환한다.
		pEntity->ReleaseQueRef();
	}
}

void CTile::TileBroadCast()
{
	// Broadcast 큐를 매 Tick 소비해 패킷과 선택적 제외 Entity Ref를 함께 반환한다.
	m_vecBroadCastBuffer.clear();
	m_queueBroadCast.PopVector(m_vecBroadCastBuffer);
	for (st_TileBroadCast& job : m_vecBroadCastBuffer)
	{
		Broadcast(&job.packet, job.pEntity);
		if (job.pEntity != nullptr)
			job.pEntity->ReleaseQueRef();
	}
}

void CTile::Init(CMainWorld* parent, COORDINATE coord)
{
	m_parent = parent;
	m_Coord = coord;

	// 일반적인 소규모 작업은 추가 할당 없이 처리하도록 초기 용량을 확보한다.
	m_vecJobBuffer.reserve(32);
	m_vecBroadCastBuffer.reserve(32);
	m_vecPreviousPlayers.reserve(8);
}

void CTile::EnqueueJob(int type, CEntity* pEntity)
{
	if (pEntity == nullptr)
		return;

	// Tile 작업이 처리될 때까지 대상 Entity의 재사용을 Queue Ref로 차단한다.
	pEntity->AddQueRef();
	m_queue.Push({ type, pEntity });
}

void CTile::EnqueueBroadCast(CEntity* pEntity, CPacket* Packet)
{
	if (Packet == nullptr)
		return;

	// nullptr은 제외 대상이 없는 전체 Broadcast이므로 Entity Ref만 조건부로 획득한다.
	if (pEntity != nullptr)
		pEntity->AddQueRef();
	m_queueBroadCast.Push({ pEntity, *Packet });
}

bool CTile::AddPlayer(CEntity* pEntity)
{
	if (pEntity == nullptr || m_vecPlayer.Contains(pEntity))
		return false;

	CaptureAoiSnapshot();
	bool ret = m_vecPlayer.AddEntity(pEntity);
	if (ret)
	{
		m_iActive.fetch_add(1);
		pEntity->SetTilePos(m_Coord);
		// 전체 AOI 검사의 실행 여부는 Debug 빌드에서만 추적한다.
#ifdef __DEBUG__
		m_parent->MarkAoiDirty();
#endif
		
		//g_LogGame.DLog("Enter Tile[%d,%d] ActiveCount : %d", m_Coord.X, m_Coord.Z, m_iActive.load());
#ifdef __DEBUG__
		int PlayerCount = m_vecPlayer.GetSize();
		if (m_iActive.load() != PlayerCount)
		{
			g_LogGame.DLog("Add Not Equel Active[%d] != PlayerCount[%d]", m_iActive.load(), PlayerCount);
		}
#endif // __DEBUG__
	}
	return ret;
}

bool CTile::RemovePlayer(CEntity* pEntity)
{
	if (pEntity == nullptr)
		return false;

	if (!m_vecPlayer.Contains(pEntity))
	{
		// 현재 자신의 타일에 없음
		COORDINATE curTilePos = pEntity->GetTilePos();
		if (curTilePos != m_Coord)
		{
			g_LogGame.ELog("Error Wrong Tile Remove");
			CTile* pTile = m_parent->GetTile(curTilePos);
			// 저장된 Tile이 유효할 때만 잘못 전달된 제거 작업을 실제 소유 Tile로 넘긴다.
			if (pTile != nullptr && pTile != this)
				pTile->EnqueueJob(ETILE_JOB_TYPE::WRONG_ENTITY_REMOVE, pEntity);
		}
		return false;
	}

	CaptureAoiSnapshot();
	bool ret = m_vecPlayer.RemoveEntity(pEntity);
	if (ret)
	{
		m_iActive.fetch_sub(1);
		// 전체 AOI 검사의 실행 여부는 Debug 빌드에서만 추적한다.
#ifdef __DEBUG__
		m_parent->MarkAoiDirty();
#endif

		//g_LogGame.DLog("Leave Tile[%d,%d] ActiveCount : %d", m_Coord.X, m_Coord.Z, m_iActive.load());
#ifdef __DEBUG__
		int PlayerCount = m_vecPlayer.GetSize();
		if (m_iActive.load() != PlayerCount)
		{
			g_LogGame.DLog("Remove Not Equel Active[%d] != PlayerCount[%d]", m_iActive.load(), PlayerCount);
		}
#endif // __DEBUG__
	}
	return ret;
}

void CTile::Update()
{
	TileJobRun();
	TileBroadCast();
}

void CTile::CaptureAoiSnapshot()
{
	if (m_bAoiSnapshotCaptured)
		return;

	m_vecPreviousPlayers.clear();
	const std::vector<CEntity*>& currentPlayers = m_vecPlayer.GetVector();
	m_vecPreviousPlayers.reserve(currentPlayers.size());
	for (CEntity* pEntity : currentPlayers)
	{
		// AOI 단계가 끝날 때까지 이전 Tile 구성의 Entity 수명을 유지한다.
		pEntity->AddQueRef();
		m_vecPreviousPlayers.push_back(pEntity);
	}
	m_bAoiSnapshotCaptured = true;
}

void CTile::AppendCurrentPlayers(std::vector<CEntity*>& players) const
{
	const std::vector<CEntity*>& tilePlayers = m_vecPlayer.GetVector();
	players.insert(players.end(), tilePlayers.begin(), tilePlayers.end());
}

void CTile::AppendPreviousPlayers(std::vector<CEntity*>& players) const
{
	const std::vector<CEntity*>& tilePlayers = m_bAoiSnapshotCaptured
		? m_vecPreviousPlayers
		: m_vecPlayer.GetVector();
	players.insert(players.end(), tilePlayers.begin(), tilePlayers.end());
}

void CTile::ClearAoiSnapshot()
{
	if (!m_bAoiSnapshotCaptured)
		return;

	for (CEntity* pEntity : m_vecPreviousPlayers)
		pEntity->ReleaseQueRef();
	m_vecPreviousPlayers.clear();
	m_bAoiSnapshotCaptured = false;
}

void CTile::Broadcast(CPacket* pPacket, CEntity* pEntity)
{
	const std::vector<CEntity*>& vec = m_vecPlayer.GetVector();
	int Loop = static_cast<int>(vec.size());
	
	if (Loop == 0)
		return;

	for (int i = 0; i < Loop; i++)
	{
		if (vec[i] == pEntity)
			continue;
		((CPlayer*)vec[i])->SendPacket(pPacket);
	}

	//g_LogServer.DLog("Tile[%d,%d] Broadcast Packet Size : %d TileCount : %d", m_Coord.X, m_Coord.Z, pPacket->GetDataSize(), Loop);
}
