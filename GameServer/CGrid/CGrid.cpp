#include "CGrid.h"

#include "../PacketProc.h"
#include "../NetWork/CNetServer.h"
#include "../Stub/StructDef.h"
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
		if(ret)
			vecCompleteMove.push_back(vec[i]);

		// Move 완료 여부와 상관 없이 Tile, Grid Update
		COORDINATE curTilePos = vec[i]->GetTilePos();
		COORDINATE newTilePos = m_parent->CalCoord(vec[i]->GetPosition());

		// 현재 TilePos 와 새 TilePos 가 다른 경우
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
			COORDINATE OutOfRangeAOI = { curTilePos.X - (diff.X), curTilePos.Z - (diff.Z) };	// 벗어난 AOI
			COORDINATE InOfRangeAOI = { newTilePos.X + (diff.X), newTilePos.Z + (diff.Z) };		// 들어간 AOI 
			
			// 범위 밖으로 나감
			st_STC_AoiOutPlayer outAoiPlayer;
			outAoiPlayer.ID = vec[i]->GetID();
			CPacket outAoiPacket;
			outAoiPacket << outAoiPlayer;

			// 범위 안으로 들어옴
			st_STC_AoiInPlayer inAoiPlayer;
			inAoiPlayer.info.ID = vec[i]->GetID();
			inAoiPlayer.info.pos = vec[i]->GetPosition();
			inAoiPlayer.info.speed = vec[i]->GetMoveSpeed();
			
			CPacket inAoiPacket;
			inAoiPacket << inAoiPlayer;
			
			if (vec[i]->GetMoveState() != eMOVESTATE::STOPPED)
			{
				st_STC_OtherMoveStart inAoiPlayerMove;
				inAoiPlayerMove.type = vec[i]->GetType();
				inAoiPlayerMove.ID = vec[i]->GetID();
				inAoiPlayerMove.dir = vec[i]->GetDirVector();
				inAoiPlayerMove.pos = vec[i]->GetPosition();

				inAoiPacket << inAoiPlayerMove;
			}
			
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
					pOut->EnqueueBroadCast(nullptr, &outAoiPacket);						// 다른 유저에게 삭제 메시지
					pOut->EnqueueJob(ETILE_JOB_TYPE::NOTIFY_TILE_REMOVE_AOI, vec[i]);	// 나에게 다른 유저 지우기 메시지
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
					pIn->EnqueueBroadCast(nullptr, &inAoiPacket);						// 다른 유저에게 생성 메시지
					pIn->EnqueueJob(ETILE_JOB_TYPE::NOTIFY_TILE_ENTER_AOI, vec[i]);		// 나에게 다른 유저 생성 메시지
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
					pOut->EnqueueBroadCast(nullptr, &outAoiPacket);						// 다른 유저에게 삭제 메시지
					pOut->EnqueueJob(ETILE_JOB_TYPE::NOTIFY_TILE_REMOVE_AOI, vec[i]);	// 나에게 다른 유저 지우기 메시지
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
					pIn->EnqueueBroadCast(nullptr, &inAoiPacket);						// 다른 유저에게 생성 메시지
					pIn->EnqueueJob(ETILE_JOB_TYPE::NOTIFY_TILE_ENTER_AOI, vec[i]);		// 나에게 다른 유저 생성 메시지
				}
			}

			this->RemovePlayer(vec[i]);
			pCurTile->RemovePlayer(vec[i]);

			// 같은 Grid 에서 관리 할때
			if (pCurTile->GetManagementGrid() == pNewTile->GetManagementGrid())
			{
				pNewTile->AddPlayer(vec[i]);
			}
			// 다른 Grid 에서 관리 할때
			else
			{
				CGrid* pNewGrid = m_parent->GetGrid(pNewTile->GetManagementGrid());
				pNewGrid->EnqueueEntityJob(EGRID_ADD_TYPE::CHANGE_THREAD, vec[i]);
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
	st_GridJob msg;
	while (m_queueEntity.POP(msg))
	{
		switch (msg.type)
		{
		case EGRID_ADD_TYPE::ENTER_ZONE:
		{
			OnEnterZone(msg.pEntity);
		}
			break;
		case EGRID_ADD_TYPE::LEAVE_ZONE:
		{
			OnLeaveZone(msg.pEntity);
		}
			break;
		case EGRID_ADD_TYPE::ADD_TELEPORT:
		{
			OnTeleport(msg.pEntity);
		}
			break;
		case EGRID_ADD_TYPE::CHANGE_THREAD:
		{
			OnChangeThread(msg.pEntity);
		}
			break;
		default:
			g_LogGame.ELog("ERROR msg Change Grid type: %d", msg.type);
			break;
		}
	}
}

void CGrid::OnEnterZone(CEntity* pEntity)
{
	AddPlayer(pEntity);
	CTile* pTile = m_parent->GetTile(pEntity->GetPosition());
	if (pTile == nullptr)
	{
		g_LogGame.ELog("ERROR EnterZone");
		return;
	}

	pTile->AddPlayer(pEntity);
	
	SendInitAOITile(pTile->GetCoord(), pEntity);	
}

void CGrid::OnLeaveZone(CEntity* pEntity)
{
	RemovePlayer(pEntity);
	CTile* pTile = m_parent->GetTile(pEntity->GetPosition());
	if (pTile == nullptr)
	{
		g_LogGame.ELog("ERROR LeaveZone");
		return;
	}
	pTile->RemovePlayer(pEntity);

	SendRemoveAOITile(pTile->GetCoord(), pEntity);
}

void CGrid::OnTeleport(CEntity* pEntity)
{
	AddPlayer(pEntity);
	CTile* pTile = m_parent->GetTile(pEntity->GetPosition());
	if (pTile == nullptr)
	{
		g_LogGame.ELog("ERROR Teleport");
		return;
	}
	pTile->AddPlayer(pEntity);
	
	SendInitAOITile(pTile->GetCoord(), pEntity);
}

void CGrid::OnChangeThread(CEntity* pEntity)
{
	AddPlayer(pEntity);
	CTile* pTile = m_parent->GetTile(pEntity->GetPosition());
	if (pTile == nullptr)
	{
		g_LogGame.ELog("ERROR Teleport");
		return;
	}
	pTile->AddPlayer(pEntity);

	if (pEntity->GetMoveState() == eMOVESTATE::STOPPED)
		return;

	AddMoveVector(pEntity);
}

void CGrid::Init(int id, CMainWorld* pParent)
{
	m_iID = id;
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

void CGrid::EnqueueEntityJob(int type, CEntity* pEntity)
{
	m_queueEntity.Push({ type, pEntity });
}

void CGrid::AddMoveVector(CEntity* pEntity)
{
	m_vecMove.AddEntity(pEntity);
}

void CGrid::RemoveMoveVector(CEntity* pEntity)
{
	m_vecMove.RemoveEntity(pEntity);
}

void CGrid::ProcessPacket()
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
}

void CGrid::Update()
{
	EntityJobRun();
	EntityMoveRun();

	for (int i = 0; i < m_iTileCount; i++)
	{
		m_vecTiles[i]->Update();
	}
}

bool CGrid::AddPlayer(CEntity* pEntity)
{
	pEntity->SetGridID(m_iID);
	
	if (!m_vecPlayer.AddEntity(pEntity))
		return false;

	((CPlayer*)pEntity)->AddRef();
	return true;
}

bool CGrid::RemovePlayer(CEntity* pEntity)
{
	if (!m_vecPlayer.RemoveEntity(pEntity))
		return false;
	
	m_vecMove.RemoveEntity(pEntity);
	((CPlayer*)pEntity)->ReleaseRef();

	return true;
}

void CGrid::SendInitAOITile(COORDINATE& pivot, CEntity* pEntity)
{
	st_STC_AoiInPlayer res;
	res.info.ID = pEntity->GetID();
	res.info.pos = pEntity->GetPosition();
	res.info.speed = pEntity->GetMoveSpeed();

	CPacket cPacket;
	cPacket << res;

	// 해당 Player 에게 해당 Grid 에 있는 Player 들의 정보 보내기
	for (int z = -AOI_VIEW_COUNT; z <= AOI_VIEW_COUNT; z++)
	{
		for (int x = -AOI_VIEW_COUNT; x <= AOI_VIEW_COUNT; x++)
		{
			CTile* pAOITile = m_parent->GetTile({ pivot.X + x, pivot.Z + z });
			if (pAOITile == nullptr)
				continue;

			// 시야 안 정보를 나에게 보냄
			pAOITile->EnqueueJob(ETILE_JOB_TYPE::NOTIFY_TILE_ENTER_AOI, pEntity);
			// 나의 정보를 시야에 보냄
			pAOITile->EnqueueBroadCast(pEntity, &cPacket);
		}
	}
}

void CGrid::SendRemoveAOITile(COORDINATE& pivot, CEntity* pEntity)
{
	CPacket cPacket;
	st_STC_AoiOutPlayer res;
	res.ID = pEntity->GetID();

	cPacket << res;

	for (int z = -AOI_VIEW_COUNT; z <= AOI_VIEW_COUNT; z++)
	{
		for (int x = -AOI_VIEW_COUNT; x <= AOI_VIEW_COUNT; x++)
		{
			CTile* pAOITile = m_parent->GetTile({ pivot.X + x, pivot.Z + z });
			if (pAOITile == nullptr)
				continue;

			pAOITile->EnqueueBroadCast(pEntity, &cPacket);
		}
	}
}
