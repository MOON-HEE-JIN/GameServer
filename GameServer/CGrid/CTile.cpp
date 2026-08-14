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
		case ETILE_JOB_TYPE::NOTIFY_TILE_ENTER_AOI:
			NotifyEntityTileEnterAOI(pEntity);
			break;
		case ETILE_JOB_TYPE::NOTIFY_TILE_REMOVE_AOI:
			NotifyEntityTileLeaveAOI(job.pEntity);
			break;
		case ETILE_JOB_TYPE::NOTIFY_TILE_ENTER_OBJ:
			NotifyEntityTileEnterObj(pEntity);
			break;
		case ETILE_JOB_TYPE::NOTIFY_TILE_REMOVE_OBJ:
			NotifyEntityTileLeaveObj(pEntity);
			break;
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
}

void CTile::EnqueueJob(int type, CEntity* pEntity)
{
	if (pEntity == nullptr)
		return;

	// Tile 작업이 처리될 때까지 대상 Entity의 재사용을 Queue Ref로 차단한다.
	pEntity->AddQueRef();
	m_queue.Push({ type, pEntity});
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
	bool ret = m_vecPlayer.AddEntity(pEntity);
	if (ret)
	{
		m_iActive.fetch_add(1);
		pEntity->SetTilePos(m_Coord);
		
		g_LogGame.DLog("Enter Tile[%d,%d] ActiveCount : %d", m_Coord.X, m_Coord.Z, m_iActive.load());
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
	bool ret = m_vecPlayer.RemoveEntity(pEntity);
	if (ret)
	{
		m_iActive.fetch_sub(1);
		g_LogGame.DLog("Leave Tile[%d,%d] ActiveCount : %d", m_Coord.X, m_Coord.Z, m_iActive.load());
#ifdef __DEBUG__
		int PlayerCount = m_vecPlayer.GetSize();
		if (m_iActive.load() != PlayerCount)
		{
			g_LogGame.DLog("Remove Not Equel Active[%d] != PlayerCount[%d]", m_iActive.load(), PlayerCount);
		}
#endif // __DEBUG__
	}
	else
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
	}

	return ret;
}

void CTile::Update()
{
	TileJobRun();
	TileBroadCast();
}

void CTile::NotifyEntityTileEnterAOI(CEntity* pEntity)
{
	const std::vector<CEntity*>& vec = m_vecPlayer.GetVector();
	int Loop = static_cast<int>(vec.size());

	st_STC_AoiInPlayers infos;
	st_STC_AoiInPlayerMoves moves;
	ZeroMemory(&infos, sizeof(infos));
	ZeroMemory(&moves, sizeof(moves));

	int index = 0;
	int movecount = 0;
	
	for (int i = 0; i < Loop; i++)
	{
		if (vec[i] == pEntity)
			continue;
		infos.info[index].ID = vec[i]->GetID();
		infos.info[index].pos = vec[i]->GetPosition();
		infos.info[index].speed = vec[i]->GetMoveSpeed();

		if (vec[i]->GetMoveState() != eMOVESTATE::STOPPED)
		{
			moves.move[movecount].ID = vec[i]->GetID();
			moves.move[movecount].pos = vec[i]->GetPosition();
			moves.move[movecount].dir = vec[i]->GetDirVector();
			movecount++;
		}

		index++;
		if (index > 49)
		{
			infos.Loop1 = index;
			((CPlayer*)pEntity)->SendPacket(infos);
			ZeroMemory(&infos, sizeof(infos));

			moves.Loop1 = movecount;
			((CPlayer*)pEntity)->SendPacket(moves);
			ZeroMemory(&moves, sizeof(moves));

			index = 0;
			movecount = 0;
		}
	}

	if (index > 0)
	{
		((CPlayer*)pEntity)->SendPacket(infos);

		if (movecount > 0)
		{
			moves.Loop1 = movecount;
			((CPlayer*)pEntity)->SendPacket(moves);
		}
	}
}

void CTile::NotifyEntityTileLeaveAOI(CEntity* pEntity)
{
	const std::vector<CEntity*>& vec = m_vecPlayer.GetVector();
	int Loop = static_cast<int>(vec.size());

	st_STC_AoiOutPlayers res;
	ZeroMemory(&res, sizeof(res));
	int index = 0;
	for (int i = 0; i < Loop; i++)
	{
		if (vec[i] == pEntity)
			continue;
		res.info[index].ID = vec[i]->GetID();
		res.info[index].pos = vec[i]->GetPosition();
		res.info[index].speed = vec[i]->GetMoveSpeed();

		index++;
		res.Loop1++;
		if (res.Loop1 > 49)
		{
			((CPlayer*)pEntity)->SendPacket(res);
			index = 0;
			ZeroMemory(&res, sizeof(res));
		}
	}

	if (index > 0)
	{
		((CPlayer*)pEntity)->SendPacket(res);
	}
}

void CTile::NotifyEntityTileEnterObj(CEntity* pEntity)
{
	st_STC_AoiInPlayer res;
	res.info.ID = pEntity->GetID();
	res.info.pos = pEntity->GetPosition();
	res.info.speed = pEntity->GetMoveSpeed();

	CPacket cPacket;
	cPacket << res;

	Broadcast(&cPacket, pEntity);
}

void CTile::NotifyEntityTileLeaveObj(CEntity* pEntity)
{
	st_STC_AoiOutPlayer res;
	res.ID = pEntity->GetID();

	CPacket cPacket;
	cPacket << res;

	Broadcast(&cPacket, pEntity);
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
