#include "CTile.h"

#include <Windows.h>
#include "../Log/CLog.h"
#include "../CPlayer.h"
#include "../MainWorld/CMainWorld.h"

void CTile::TileJobRun()
{
	std::vector<st_TileJob> vec;
	m_queue.PopVector(vec);
	for (st_TileJob& job : vec)
	{
		CEntity* pEntity = job.pEntity;
		if (pEntity == nullptr)
			continue;

		switch (job.type)
		{
		case ETILE_JOB_TYPE::NOTIFY_TILE_ENTER_AOI:
		{
			NotifyEntityTileEnterAOI(pEntity);
		}
			break;
		case ETILE_JOB_TYPE::BROADCAST_ENTITY_INFO:
		{
			st_STC_AoiInPlayer res;
			res.info.ID = pEntity->GetID();
			res.info.pos = pEntity->GetPosition();
			res.info.speed = pEntity->GetMoveSpeed();

			CPacket cPacket;
			cPacket << res;

			Broadcast(&cPacket, pEntity);
		}
			break;
		case ETILE_JOB_TYPE::BROADCAST_ENTITY_REMOVE:
		{
			st_STC_AoiOutPlayer res;
			res.ID = pEntity->GetID();

			CPacket cPacket;
			cPacket << res;

			Broadcast(&cPacket, pEntity);
		}
		case ETILE_JOB_TYPE::NOTIFY_TILE_REMOVE_AOI:
			NotifyEntityTileLeaveAOI(job.pEntity);
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
	std::vector<st_TileBroadCast> vec;
	m_queueBroadCast.PopVector(vec);
	for (st_TileBroadCast& job : vec)
	{
		Broadcast(&job.packet, job.pEntity);
	}
}

void CTile::Init(CMainWorld* parent, COORDINATE coord, st_Vector3F start, st_Vector3F end)
{
	m_parent = parent;
	m_Coord = coord;

	m_StartPos = start;
	m_EndPos = end;

	//g_LogGame.DLog("Create Tile[%d,%d] size[%d,%d]", coordX, coordZ, tilew, tileh);
}

void CTile::EnqueueJob(int type, CEntity* pEntity)
{
	m_queue.Push({ type, pEntity});
}

void CTile::EnqueueBroadCast(CEntity* pEntity, CPacket* Packet)
{
	if (pEntity == nullptr)
		return;

	pEntity->AddQueRef();
	m_queue.Push({ type, pEntity });
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
			pTile->EnqueueJob(ETILE_JOB_TYPE::WRONG_ENTITY_REMOVE, pEntity);
		}
	}

	return ret;
}

void CTile::Update()
{
	TileJobRun();
}

void CTile::NotifyEntityTileEnterAOI(CEntity* pEntity)
{
	const std::vector<CEntity*>& vec = m_vecPlayer.GetVector();
	int Loop = static_cast<int>(vec.size());

	st_STC_AoiInPlayers res;
	res.Loop1;
	res.info;
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

void CTile::NotifyEntityTileLeaveAOI(CEntity* pEntity)
{
	const std::vector<CEntity*>& vec = m_vecPlayer.GetVector();
	int Loop = static_cast<int>(vec.size());

	st_STC_AoiOutPlayers res;
	res.Loop1;
	res.info;
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

	g_LogServer.DLog("Tile[%d,%d] Broadcast Packet Size : %d TileCount : %d", m_Coord.X, m_Coord.Z, pPacket->GetDataSize(), Loop);
}
