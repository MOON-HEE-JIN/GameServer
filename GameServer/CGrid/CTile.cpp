#include "CTile.h"

#include <Windows.h>
#include "../Log/CLog.h"
#include "../CPlayer.h"

void CTile::TileJobRun()
{
	std::vector<st_TileJob> vec;
	m_queue.PopVector(vec);
	for (st_TileJob& job : vec)
	{
		switch (job.type)
		{
		case ETILE_JOB_TYPE::NOTIFY_TILE_ENTER_AOI:
			NotifyEntityTileEnterAOI(job.pEntity);
			break;
		case ETILE_JOB_TYPE::BROADCAST_ENTITY_INFO:
		{
			st_STC_AoiInPlayer res;
			res.info.ID = job.pEntity->GetID();
			res.info.pos = job.pEntity->GetPosition();
			res.info.speed = job.pEntity->GetMoveSpeed();

			CPacket cPacket;
			cPacket << res;

			Broadcast(&cPacket, job.pEntity);
		}
			break;
		case ETILE_JOB_TYPE::BROADCAST_ENTITY_REMOVE:
		{
			st_STC_AoiOutPlayer res;
			res.ID = job.pEntity->GetID();

			CPacket cPacket;
			cPacket << res;

			Broadcast(&cPacket, job.pEntity);
		}
			break;
		default:
			break;
		}
	}
}

void CTile::Init(COORDINATE coord, st_Vector3F start, st_Vector3F end)
{
	m_Coord = coord;

	m_StartPos = start;
	m_EndPos = end;

	//g_LogGame.DLog("Create Tile[%d,%d] size[%d,%d]", coordX, coordZ, tilew, tileh);
}


void CTile::Enqueue(int type, CEntity* pEntity)
{
	m_queue.Push({ type, pEntity });
}

bool CTile::AddPlayer(CEntity* pEntity)
{
	bool ret = m_vecPlayer.AddEntity(pEntity);
	if (ret)
	{
		m_iActive.fetch_add(1);
		pEntity->SetTilePos(m_Coord);
		((CPlayer*)pEntity)->AddRef();
		
		g_LogGame.DLog("Enter Tile[%d,%d] ActiveCount : %d", m_Coord.X, m_Coord.Z, m_iActive.load());
	}
	return ret;
}

bool CTile::RemovePlayer(CEntity* pEntity)
{
	bool ret = m_vecPlayer.RemoveEntity(pEntity);
	if (ret)
	{
		m_iActive.fetch_sub(1);
		((CPlayer*)pEntity)->ReleaseRef();
		g_LogGame.DLog("Leave Tile[%d,%d] ActiveCount : %d", m_Coord.X, m_Coord.Z, m_iActive.load());
	}
	if (!ret)
	{
		g_LogGame.ELog("ERROR LEVEL");
	}
	return ret;
}

void CTile::Update()
{
	TileJobRun();
	if (m_iDebugLogTime + m_iDebugLogDelayTime < GetTickCount())
	{
		if (m_iActive.load() > 0)
		{
			//g_LogGame.DLog("Log Tile[%d,%d] ActiveCount : %d", m_Coord.X, m_Coord.Z, m_iActive.load());
		}
		m_iDebugLogTime = GetTickCount();
	}
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

void CTile::Broadcast(CPacket* pPacket, CEntity* pEntity)
{
	const std::vector<CEntity*>& vec = m_vecPlayer.GetVector();
	int Loop = static_cast<int>(vec.size());
	for (int i = 0; i < Loop; i++)
	{
		if (vec[i] == pEntity)
			continue;
		((CPlayer*)vec[i])->SendPacket(pPacket);
	}
}
