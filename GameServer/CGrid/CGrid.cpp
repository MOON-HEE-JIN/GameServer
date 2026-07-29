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

		if (curTilePos == newTilePos)
			continue;

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

		// Tile 이 변경 된 상태 관리 주체를 변경 해야 한다
		// 현재 Grid, Tile 에서 제거
		// 새로운 Grid, Tile 에 추가

		// 더이상 해당 Tile 에서 관리 하지 않음
		pCurTile->RemovePlayer(vec[i]);

		// 같은 Grid 에서 관리 할때
		if (pCurTile->GetManagementGrid() == pNewTile->GetManagementGrid())
		{
			g_LogGame.DLog("이동 으로 인한 같은 Thread 내 의 Tile 변경");
			pNewTile->AddPlayer(vec[i]);
		}
		else
		{
			CGrid* pNewGrid = m_parent->GetGrid(pNewTile->GetManagementGrid());
			if(pNewGrid == nullptr)
			{
				g_LogGame.ELog("ERROR Change Thread GridID : %d", pNewTile->GetManagementGrid());
				continue;
			}
			m_vecChangeThreadMove.push_back({ vec[i], pNewGrid });
		}
	}

	Loop = static_cast<int>(vecCompleteMove.size());
	for (int i = 0; i < Loop; i++)
	{
		RemoveMoveVector(vecCompleteMove[i]);
	}

	Loop = static_cast<int>(m_vecChangeThreadMove.size());
	for (int i = 0; i < Loop; i++)
	{
		RemovePlayer(m_vecChangeThreadMove[i].pEntity);
		m_vecChangeThreadMove[i].pGrid->EnqueueEntityJob(EGRID_ADD_TYPE::CHANGE_THREAD_REQUEST, m_vecChangeThreadMove[i].pEntity);
	}
}

void CGrid::EntityJobRun()
{
	st_GridJob msg;
	while (m_queueEntity.POP(msg))
	{
		CEntity* pEntity = msg.pEntity;
		if (pEntity == nullptr)
			continue;

		switch (msg.type)
		{
		case EGRID_MSG_TYPE::GRID_MSG_SPAWN:
		{
			OnSpawnGrid(pEntity);
		}
		break;
		case EGRID_MSG_TYPE::GRID_MSG_ENTER:
		{
			OnEnterGrid(pEntity);
		}
			break;
		case EGRID_MSG_TYPE::GRID_MSG_LEAVE:
		{
			OnLeaveGrid(pEntity);
		}
			break;
		case EGRID_ADD_TYPE::CHANGE_THREAD_REQUEST:
		{
			OnChangeThread(msg.pEntity);
		}
			break;
		case EGRID_ADD_TYPE::CHANGE_THREAD_REQUEST_NO:
		{
			AddPlayer(msg.pEntity);
		}
			break;
		case EGRID_ADD_TYPE::CHANGE_THREAD_REQUEST_OK:
			break;
		default:
			g_LogGame.ELog("ERROR msg Change Grid type: %d", msg.type);
			break;
		}

		// EnqueueEntityJob 에서 획득한 작업 참조를 반환한다.
		pEntity->ReleaseQueRef();
	}
}

void CGrid::OnSpawnGrid(CEntity* pEntity)
{
	OnEnterGrid(pEntity);
	if (pEntity->GetEntityType() == eENTITY_TYPE::ENTITY_PLAYER)
	{
		CPlayer* pPlayer = (CPlayer*)pEntity;
		// 새로운 Zone 에 입장 완료
		pEntity->SetZoneStatus(eZONESTATUS::STABLE);
		{
			st_STC_ChangeZone pack;
			pack.ret = 0;
			pack.channel = pPlayer->GetChannel();
			pack.zone = pPlayer->GetZoneID();

			pPlayer->SendPacket(pack);
		}
	}
}

void CGrid::OnEnterGrid(CEntity* pEntity)
{
	CTile* pTile = m_parent->GetTile(pEntity->GetPosition());
	if (pTile == nullptr)
	{
		g_LogGame.ELog("ERROR EnterZone");
		return;
	}

	if (!AddPlayer(pEntity))
	{
		g_LogGame.ELog("ERROR Duplicate EnterGrid Entity:%d", pEntity->GetID());
		return;
	}

	if (!pTile->AddPlayer(pEntity))
	{
		RemovePlayer(pEntity);
		g_LogGame.ELog("ERROR Duplicate EnterTile Entity:%d", pEntity->GetID());
		return;
	}

	SendInitAOITile(pTile->GetCoord(), pEntity);
}
void CGrid::OnLeaveGrid(CEntity* pEntity)
{
	bool bGridRemoved = RemovePlayer(pEntity);

	CTile* pTile = m_parent->GetTile(pEntity->GetPosition());
	if (pTile == nullptr)
	{
		g_LogGame.ELog("ERROR LeaveZone");
		return;
	}

	bool bTileRemoved = pTile->RemovePlayer(pEntity);

	if (bGridRemoved || bTileRemoved)
		SendRemoveAOITile(pTile->GetCoord(), pEntity);
	else
		g_LogGame.ELog("ERROR LeaveGrid Entity:%d", pEntity->GetID());
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
	if (pEntity == nullptr)
		return;

	// 큐에서 처리될 때까지 Player 재사용을 막는다.
	pEntity->AddQueRef();
	m_queueEntity.Push({ type, pEntity });
}

bool CGrid::AddMoveVector(CEntity* pEntity)
{
	return m_vecMove.AddEntity(pEntity);
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
		CPlayer* pPlayer = g_Net.GetPlayer(job.PlayerHandle, job.SessionHandle);
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

	return true;
}

bool CGrid::RemovePlayer(CEntity* pEntity)
{
	if (!m_vecPlayer.RemoveEntity(pEntity))
		return false;
	
	m_vecMove.RemoveEntity(pEntity);

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
