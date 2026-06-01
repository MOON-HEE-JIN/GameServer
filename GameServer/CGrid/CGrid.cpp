#include "CGrid.h"

#include "../PacketProc.h"
#include "../NetWork/CNetServer.h"
#include "../Stub/EnumDef.h"
#include "../CPlayer.h"
#include "../Log/CLog.h"

static PacketProc proc;

CGrid::CGrid()
{
	m_iTileCount = 0;
}

CGrid::~CGrid()
{

}

void CGrid::EntityJobRun()
{
	st_AddMsg msg;
	while (m_queueEntity.POP(msg))
	{
		switch (msg.type)
		{
		case EGRID_ADD_TYPE::GRID_ENTER:
			AddPlayer(msg.pEntity);
			break;
		case EGRID_ADD_TYPE::GRID_LEAVE:
			RemovePlayer(msg.pEntity);
			break;
		case EGRID_ADD_TYPE::ADD_TELEPORT:
		{
			OnTeleport(msg.pEntity);
		}
		break;
		case EGRID_ADD_TYPE::SUB:
			RemovePlayer(msg.pEntity);
			break;
		default:
			g_LogGame.ELog("ERROR msg Change Grid type: %d", msg.type);
			break;
		}
	}
}

void CGrid::OnTeleport(CEntity* pEntity)
{
	AddPlayer(pEntity);

	st_STC_Teleport res;
	res.ret = 0;
	((CPlayer*)pEntity)->SendPacket(res);
}

void CGrid::OnRegisterTile(CTile* pTile)
{
	m_iTileCount++;
	m_vecTiles.push_back(pTile);
	pTile->OnReigsterGrid(m_iID);
}

void CGrid::EnqueueProcJob(PROC_MSG& msg)
{
	m_queueProc.Enqueue(msg);
}

void CGrid::EnqueueEntityJob(int type, int key, CEntity* pEntity)
{
	m_queueEntity.Push({ type, key, pEntity });
}

void CGrid::AddMoveVector(CEntity* pEntity)
{
	m_vecMove.AddEntity(pEntity);
}

void CGrid::RemoveMoveVector(CEntity* pEntity)
{
	m_vecMove.RemoveEntity(pEntity);
}

void CGrid::Update()
{
	PROC_MSG job;
	while (m_queueProc.TryDequeue(job))
	{
		CPlayer* pPlayer = g_Net.GetPlayer(job.PlayerHandle);
		if (pPlayer == nullptr)
			continue;
		if (pPlayer->GetZoneStatus() != eZONESTATUS::STABLE)
		{
			st_STC_ChangeingZone res;
			res.ret = ERROR_CODE::ZONE_CHANEING;
			res.type = job.type;

			pPlayer->SendPacket(res);
			continue;
		}

		proc.DO_GAME_Proc(job.type, pPlayer, job.packet);
	}
	
	EntityJobRun();
	
	for (int i = 0; i < m_iTileCount; i++)
	{
		m_vecTiles[i]->Update();
	}
}

bool CGrid::AddPlayer(CEntity* pEntity)
{
	pEntity->SetGridID(m_iID);
	
	return m_vecPlayer.AddEntity(pEntity);
}

bool CGrid::RemovePlayer(CEntity* pEntity)
{
	m_vecMove.RemoveEntity(pEntity);
	pEntity->SetGridID(-1);
	return m_vecPlayer.RemoveEntity(pEntity);
}
