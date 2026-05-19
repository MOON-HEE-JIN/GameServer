#include "CGrid.h"

#include "../PacketProc.h"
#include "../NetWork/CNetServer.h"
#include "../Stub/EnumDef.h"
#include "../CPlayer.h"
#include "../Log/CLog.h"

static PacketProc proc;

CGrid::CGrid()
{
}

CGrid::~CGrid()
{
	delete[] m_Tiles;
}

void CGrid::Init(int width, int height, int gridsizeW, int gridsizeH, st_Vector3F origin)
{
	// 전체 크기
	m_iWidth = width;
	m_iHeight = height;
	// 관장 하는 크기
	m_iGridSizeW = gridsizeW;
	m_iGridSizeH = gridsizeH;

	m_stOrigin = origin;

	m_iTileCountW = 4;
	m_iTileCountH = 4;

	m_iTileCount = 4 * 4;

	m_iTileSizeW = m_iGridSizeW / m_iTileCountW;
	m_iTileSizeH = m_iGridSizeH / m_iTileCountH;

	m_Tiles = new CTile[m_iTileCountH * m_iTileCountW];

	COORDINATE coord = {0,0};
	for (int i = 0; i < m_iTileCount; i++)
	{
		m_Tiles[i].Init(coord.X, coord.Z, m_iTileSizeW, m_iTileSizeH);
		if (++coord.X >= 4)
		{
			coord.X = 0;
			coord.Z++;
		}
	}
}

void CGrid::AddMsgProc()
{
	st_AddMsg msg;
	while (m_AddQueue.POP(msg))
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
			AddPlayer(msg.pEntity);
			st_STC_Teleport res;
			res.ret = 0;
			((CPlayer*)msg.pEntity)->SendPacket(res);
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

void CGrid::MoveUpdate()
{
	const std::vector<CEntity*> mvec = m_MoveVector.GetVector();
	std::vector<CEntity*> movecomplete;
	int Loop = static_cast<int>(mvec.size());
	for (int i = 0; i < Loop; i++)
	{
		if (mvec[i]->MoveUpdate())
		{
			movecomplete.push_back(mvec[i]);
		}
	}
	Loop = static_cast<int>(movecomplete.size());
	for (int i = 0; i < Loop; i++)
	{
		m_MoveVector.RemoveEntity(movecomplete[i]);
	}
}

void CGrid::Update(void* pMainWorld)
{
	PROC_MSG job;
	while (m_queue.TryDequeue(job))
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

	AddMsgProc();
	
	MoveUpdate();
	

	for (int i = 0; i < m_iTileCount; i++)
	{
		m_Tiles[i].Update();
	}
}

bool CGrid::AddPlayer(CEntity* pEntity)
{
	CPlayer* pPlayer = (CPlayer*)pEntity;
	
	st_Vector3F pos = pEntity->GetPosition();
	COORDINATE NewGridPos = {static_cast<int>(pos.X) / m_iGridSizeW, static_cast<int>(pos.Y) / m_iGridSizeH};
	if (pPlayer->GetGridPos() != NewGridPos)
		pPlayer->SetGridPos(NewGridPos);

	st_Vector3F localPos = pos - m_stOrigin;

	int tileX = static_cast<int>(localPos.X) / m_iTileSizeW;
	int tileZ = static_cast<int>(localPos.Z) / m_iTileSizeH;

	if (!m_Tiles[tileZ * m_iTileCountW + tileX].AddPlayer(pPlayer->GetID(), pPlayer))
		return false;

	// 주위 에 생성 broadcast 필요

	return true;
}

bool CGrid::EnqueueAddPlayer(int type, CEntity* pEntity)
{
	if (EGRID_ADD_TYPE::END < type || type < 0)
		return false;
	st_AddMsg msg = {type, pEntity};

	m_AddQueue.Push(msg);
	return true;
}

bool CGrid::RemovePlayer(CEntity* pEntity)
{
	CPlayer* pPlayer = (CPlayer*)pEntity;
	st_Vector3F pos = pEntity->GetPosition();

	st_Vector3F localPos = pos - m_stOrigin;

	int tileX = static_cast<int>(localPos.X) / m_iTileSizeW;
	int tileZ = static_cast<int>(localPos.Z) / m_iTileSizeH;

	if (!m_Tiles[tileZ * m_iTileCountW + tileX].RemovePlayer(pPlayer->GetID(), pPlayer))
		return false;

	// 주위 에 삭제 boradcast 필요

	return true;
}

bool CGrid::EnqueueRemovePlayer(int type, CEntity* pEntity)
{
	if (EGRID_ADD_TYPE::END < type || type < 0)
		return false;
	st_AddMsg msg = { type, pEntity };

	m_AddQueue.Push(msg);
	return true;
}

void CGrid::AddMove(CEntity* pEntity)
{
	m_MoveVector.AddEntity(pEntity);
}

void CGrid::RemoveMove(CEntity* pEntity)
{
	m_MoveVector.RemoveEntity(pEntity);
}

st_Vector3F CGrid::GetCenter()
{
	return m_stOrigin + st_Vector3F(m_iGridSizeW * 0.5f, 0, m_iGridSizeH * 0.5f);
}
