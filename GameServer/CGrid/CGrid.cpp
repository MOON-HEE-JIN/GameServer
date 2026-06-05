#include "CGrid.h"

#include "../PacketProc.h"
#include "../NetWork/CNetServer.h"
#include "../Stub/EnumDef.h"
#include "../CPlayer.h"
#include "../Log/CLog.h"
#include "../MainWorld/CMainWorld.h"

static PacketProc proc;

CGrid::CGrid()
{
	m_iTileCount = 0;
}

CGrid::~CGrid()
{

}

void CGrid::EntityMoveRun()
{
	const std::vector<CEntity*>& vec = m_vecMove.GetVector();
	int Loop = static_cast<int>(vec.size());

	std::vector<CEntity*> vecCompleteMove;
	for (int i = 0; i < Loop; i++)
	{
		bool ret = vec[i]->MoveUpdate();
		
		// true. 이동이 완료된 상태
		vecCompleteMove.push_back(vec[i]);

		// 여기서 Tile 을 바꿔줘야하나 아니면 CEntity::MoveComplete 에서 Tile 의 위치를 바꿔줘야하나?
		// 중요한건 Tile 을 바꿀려면 Tile 을 알고 있는 CMainWorld 을 알고 있어야함
		COORDINATE curTilePos = vec[i]->GetTilePos();
		COORDINATE newTilePos = m_parent->CalCoord(vec[i]->GetPosition());

		if (curTilePos != newTilePos)
		{
			// Tile 변경
			CTile* pCurTile = m_parent->GetTile(curTilePos);
			CTile* pNewTile = m_parent->GetTile(newTilePos);

			COORDINATE diff = newTilePos - curTilePos;

#ifdef __DEBUG__
			// 이동 으로 인한 tile 변경은 1 로 제한 된다
			if (abs(diff.X) > 1 || abs(diff.Z) > 1)
				g_LogGame.DLog("ERROR diff > 1");
#endif // __DEBUG__

			// 기존 AOI 범위 curTilePos +- AOI_VIEW_COUNT
			// 변경 AOI 범위 newTilePos +- AOI_VIEW_COUNT
			COORDINATE OutOfRangeAOI = { curTilePos.X - (diff.X * AOI_VIEW_COUNT), curTilePos.Z - (diff.Z * AOI_VIEW_COUNT) };	// 벗어난 AOI
			COORDINATE InOfRangeAOI = { newTilePos.X + (diff.X * AOI_VIEW_COUNT), newTilePos.Z + (diff.Z * AOI_VIEW_COUNT) };	// 들어간 AOI 
			
			if (diff.X != 0)
			{
				int MinOutH = curTilePos.Z - AOI_VIEW_COUNT;
				int MaxOutH = curTilePos.Z + AOI_VIEW_COUNT;
				for (int H = MinOutH; H <= MaxOutH; H++)
				{
					CTile* pOut = m_parent->GetTile({ OutOfRangeAOI.X, H });
					if (pOut == nullptr)
						continue;

					// OutOfRangeAOI
					// 시야 밖으로 나감 판정
				}

				int MinInH = newTilePos.Z - AOI_VIEW_COUNT;
				int MaxInH = newTilePos.Z + AOI_VIEW_COUNT;
				for (int H = MinInH; H <= MaxInH; H++)
				{
					CTile* pIn = m_parent->GetTile({ InOfRangeAOI.X, H });
					if (pIn == nullptr)
						continue;

					// InOfRangeAOI
					// 시야 안으로 들어온 판정
				}
			}
			
			if (diff.Z != 0)
			{
				int MinOutW = curTilePos.X - AOI_VIEW_COUNT;
				int MaxOutW = curTilePos.X + AOI_VIEW_COUNT;
				
				// 겹치는 부분 제거
				if (diff.X < 0)
					MaxOutW--;
				else if (diff.X > 0)
					MinOutW--;

				for (int W = MinOutW; W <= MaxOutW; W++)
				{
					CTile* pOut = m_parent->GetTile({ W, OutOfRangeAOI.Z });
					if (pOut == nullptr)
						continue;

					// OutOfRangeAOI
					// 시야 밖으로 나감 판정
				}

				int MinInW = newTilePos.X - AOI_VIEW_COUNT;
				int MaxInW = newTilePos.X + AOI_VIEW_COUNT;

				// 겹치는 부분 제거
				if (diff.X < 0)
					MaxInW--;
				else if (diff.X > 0)
					MinInW--;

				for (int W = MinInW; W <= MaxInW; W++)
				{
					CTile* pIn = m_parent->GetTile({ W, InOfRangeAOI.Z });
					if (pIn == nullptr)
						continue;

					// InOfRangeAOI
					// 시야 안으로 들어온 판정
				}
			}

			// 같은 Grid 에서 관리 할때
			if (pCurTile->GetManagementGrid() == pNewTile->GetManagementGrid())
			{
				pCurTile->RemovePlayer(vec[i]);
				pNewTile->AddPlayer(vec[i]);
			}
			// 다른 Grid 에서 관리 할때
			else
			{
				
			}
		}

		if (!ret)
			continue;

		vecCompleteMove.push_back(vec[i]);
		//GetTile();
	}

	Loop = static_cast<int>(vecCompleteMove.size());
	for (int i = 0; i < Loop; i++)
	{
		RemoveMoveVector(vecCompleteMove[i]);
	}
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

void CGrid::Init(CMainWorld* pParent)
{
	m_parent = pParent;
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
		//m_vecTiles[i]->Update();
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
