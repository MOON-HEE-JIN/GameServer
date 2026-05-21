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
	// start origin --> end  orgin + {m_iTileSizeW * m_iTileCount, 0, m_iTileSizeH * m_iTileCount};
	m_stGridEndPos = origin + st_Vector3F(m_iTileSizeW* m_iTileCount, 0, m_iTileSizeH* m_iTileCount);
}

void CGrid::AddMsgProc()
{
	st_AddMsg msg;
	while (m_AddQueue.POP(msg))
	{
		switch (msg.type)
		{
		case EGRID_ADD_TYPE::GRID_ENTER:
			AddPlayer(msg.key, msg.pEntity);
			break;
		case EGRID_ADD_TYPE::GRID_LEAVE:
			RemovePlayer(msg.key, msg.pEntity);
			break;
		case EGRID_ADD_TYPE::ADD_TELEPORT:
		{
			AddPlayer(msg.key, msg.pEntity);
			st_STC_Teleport res;
			res.ret = 0;
			((CPlayer*)msg.pEntity)->SendPacket(res);
		}
		break;
		case EGRID_ADD_TYPE::SUB:
			RemovePlayer(msg.key, msg.pEntity);
			break;
		default:
			g_LogGame.ELog("ERROR msg Change Grid type: %d", msg.type);
			break;
		}
	}
}

void CGrid::MoveUpdate()
{
	const std::vector<CEntity*> mvec = m_vecEntityMove.GetVector();
	std::vector<CEntity*> movecomplete;
	int Loop = static_cast<int>(mvec.size());
	for (int i = 0; i < Loop; i++)
	{
		if (mvec[i]->MoveUpdate())
		{
			movecomplete.push_back(mvec[i]);
		}
		// 여기서 CEntity 의 Grid 및 Tile Update 를 해야한다
		st_Vector3F pos = mvec[i]->GetPosition();
		st_Vector3F localPos = pos - m_stOrigin;

		COORDINATE curGrid = mvec[i]->GetGridPos();
		COORDINATE curTile = mvec[i]->GetTilePos();

		COORDINATE newGrid = COORDINATE(pos.X / m_iGridSizeW, pos.Z / m_iGridSizeH);
		COORDINATE newTile;
		// Tile 의 변화 체크

		// Grid 의 변화 체크

	}
	Loop = static_cast<int>(movecomplete.size());
	for (int i = 0; i < Loop; i++)
	{
		m_vecEntityMove.RemoveEntity(movecomplete[i]);
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

bool CGrid::AddPlayer(int key, CEntity* pEntity)
{
	CPlayer* pPlayer = (CPlayer*)pEntity;
	
	st_Vector3F pos = pEntity->GetPosition();
	COORDINATE NewGridPos = {static_cast<int>(pos.X) / m_iGridSizeW, static_cast<int>(pos.Y) / m_iGridSizeH};
	if (pPlayer->GetGridPos() != NewGridPos)
		pPlayer->SetGridPos(NewGridPos);

	st_Vector3F localPos = pos - m_stOrigin;

	CTile* pTile = GetTile(localPos);

	if (pTile == nullptr)
	{
		g_LogGame.ELog("ERROR WRONG LocalPos [%f, %f, %f]  Origin[%f, %f, %f]"
			, localPos.X, localPos.Y, localPos.Z, m_stOrigin.X, m_stOrigin.Y, m_stOrigin.Z);
		return false;
	}

	if (!pTile->AddPlayer(key, pPlayer))
		return false;

	// 주위 에 생성 broadcast 필요
	pPlayer->AddRef();
	return true;
}

bool CGrid::EnqueueAddPlayer(int type, int key, CEntity* pEntity)
{
	if (EGRID_ADD_TYPE::END < type || type < 0)
		return false;
	st_AddMsg msg = {type, key, pEntity};

	m_AddQueue.Push(msg);
	return true;
}

bool CGrid::RemovePlayer(int key, CEntity* pEntity)
{
	CPlayer* pPlayer = (CPlayer*)pEntity;
	
	st_Vector3F pos = pEntity->GetPosition();

	st_Vector3F localPos = pos - m_stOrigin;

	CTile* pTile = GetTile(localPos);

	if (pTile == nullptr)
	{
		g_LogGame.ELog("ERROR WRONG LocalPos [%f, %f, %f]  Origin[%f, %f, %f]"
			, localPos.X, localPos.Y, localPos.Z, m_stOrigin.X, m_stOrigin.Y, m_stOrigin.Z);
		return false;
	}

	if (!pTile->RemovePlayer(key, pPlayer))
		return false;

	// 주위 에 삭제 boradcast 필요
	pPlayer->ReleaseRef();
	return true;
}

bool CGrid::EnqueueRemovePlayer(int type, int key, CEntity* pEntity)
{
	if (EGRID_ADD_TYPE::END < type || type < 0)
		return false;
	st_AddMsg msg = { type, key, pEntity };

	m_AddQueue.Push(msg);
	return true;
}

void CGrid::AddMove(CEntity* pEntity)
{
	m_vecEntityMove.AddEntity(pEntity);
}

void CGrid::RemoveMove(CEntity* pEntity)
{
	m_vecEntityMove.RemoveEntity(pEntity);
}

CTile* CGrid::GetTile(st_Vector3F localPos)
{
	int tileX = static_cast<int>(localPos.X) / m_iTileSizeW;
	int tileZ = static_cast<int>(localPos.Z) / m_iTileSizeH;

	if (tileX < 0 || tileX >= m_iTileCount || tileZ < 0 || tileZ >= m_iTileCount)	
		return nullptr;
	

	return &m_Tiles[tileZ * m_iTileCountW + tileX];
}

st_Vector3F CGrid::GetCenter()
{
	return m_stOrigin + st_Vector3F(m_iGridSizeW * 0.5f, 0, m_iGridSizeH * 0.5f);
}
