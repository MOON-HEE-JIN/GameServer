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
	// 매 Tick 사용하는 완료/전환 버퍼를 재사용해 이동 Entity 수에 따른 반복 할당을 줄인다.
	m_vecCompleteMove.reserve(128);
	m_vecGridTransfer.reserve(32);
}

CGrid::~CGrid()
{

}

void CGrid::EntityMoveRun()
{
	const std::vector<CEntity*>& vec = m_vecMove.GetVector();
	int Loop = m_vecMove.GetSize();

	m_vecCompleteMove.clear();
	m_vecGridTransfer.clear();
	for (int i = 0; i < Loop; i++)
	{
		bool ret = vec[i]->MoveUpdate();
		
		// true. 이동이 완료된 상태
		if(ret)
			m_vecCompleteMove.push_back(vec[i]);

		// Move 완료 여부와 상관 없이 Tile, Grid Update
		COORDINATE curTilePos = vec[i]->GetTilePos();
		COORDINATE newTilePos = m_parent->CalCoord(vec[i]->GetPosition());

		if (curTilePos == newTilePos)
			continue;

		// Tile 변경
		CTile* pCurTile = m_parent->GetTile(curTilePos);
		CTile* pNewTile = m_parent->GetTile(newTilePos);
		if (pCurTile == nullptr || pNewTile == nullptr)
		{
			g_LogGame.ELog("ERROR Change Tile Entity:%d", vec[i]->GetID());
			continue;
		}

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

		// 같은 Grid 에서 관리 할때
		if (pCurTile->GetManagementGrid() == pNewTile->GetManagementGrid())
		{
			// 같은 Grid의 Tile 변경은 현재 Thread에서 제거와 등록을 원자적인 순서로 처리한다.
			if (!pCurTile->RemovePlayer(vec[i]))
				continue;
			if (!pNewTile->AddPlayer(vec[i]))
			{
				pCurTile->AddPlayer(vec[i]);
				g_LogGame.ELog("ERROR Change Tile Rollback Entity:%d", vec[i]->GetID());
			}
		}
		else
		{
			CGrid* pNewGrid = m_parent->GetGrid(pNewTile->GetManagementGrid());
			if(pNewGrid == nullptr)
			{
				g_LogGame.ELog("ERROR Change Thread GridID : %d", pNewTile->GetManagementGrid());
				continue;
			}
			// 다른 Grid 전환은 순회 종료 후 기존 Move Vector를 안전하게 수정하도록 지연한다.
			m_vecGridTransfer.push_back({ vec[i], pNewGrid, pCurTile, curTilePos });
		}
	}

	Loop = static_cast<int>(m_vecCompleteMove.size());
	for (int i = 0; i < Loop; i++)
	{
		RemoveMoveVector(m_vecCompleteMove[i]);
	}

	Loop = static_cast<int>(m_vecGridTransfer.size());
	for (int i = 0; i < Loop; i++)
	{
		st_GridTransfer& transfer = m_vecGridTransfer[i];
		if (transfer.pEntity->GetEntityType() == eENTITY_TYPE::ENTITY_PLAYER &&
			static_cast<CPlayer*>(transfer.pEntity)->GetRelease())
			continue;

		if (!transfer.pSourceTile->RemovePlayer(transfer.pEntity))
		{
			continue;
		}
		if (!RemovePlayer(transfer.pEntity))
		{
			transfer.pSourceTile->AddPlayer(transfer.pEntity);
			continue;
		}

		transfer.pNewGrid->PushEntityJob(
			EGRID_MSG_TYPE::GRID_MSG_TRANSFER_IN, transfer.pEntity, m_iID, transfer.SourceTile);
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
		case EGRID_MSG_TYPE::GRID_MSG_TRANSFER_IN:
		{
			if (!OnTransferGrid(pEntity))
			{
				CGrid* pSourceGrid = m_parent->GetGrid(msg.SourceGridID);
				if (pSourceGrid != nullptr)
					pSourceGrid->EnqueueEntityJob(
						EGRID_MSG_TYPE::GRID_MSG_TRANSFER_ROLLBACK,
						pEntity, msg.SourceGridID, msg.SourceTile);
			}
		}
			break;
		case EGRID_MSG_TYPE::GRID_MSG_TRANSFER_ROLLBACK:
		{
			OnTransferRollback(pEntity, msg.SourceTile);
		}
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
	// Grid와 Tile 등록이 모두 성공한 경우에만 Spawn 완료 상태를 공개한다.
	if (!OnEnterGrid(pEntity))
		return;

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

bool CGrid::OnEnterGrid(CEntity* pEntity)
{
	CTile* pTile = m_parent->GetTile(pEntity->GetPosition());
	if (pTile == nullptr || pTile->GetManagementGrid() != m_iID)
	{
		g_LogGame.ELog("ERROR EnterZone Grid:%d", m_iID);
		return false;
	}

	if (!AddPlayer(pEntity))
	{
		g_LogGame.ELog("ERROR Duplicate EnterGrid Entity:%d", pEntity->GetID());
		return false;
	}

	if (!pTile->AddPlayer(pEntity))
	{
		RemovePlayer(pEntity);
		g_LogGame.ELog("ERROR Duplicate EnterTile Entity:%d", pEntity->GetID());
		return false;
	}

	SendInitAOITile(pTile->GetCoord(), pEntity);
	return true;
}

void CGrid::OnLeaveGrid(CEntity* pEntity)
{
	// 현재 위치가 아닌 저장된 TilePos를 사용해 실제 소유 Tile에서 제거한다.
	COORDINATE tilePos = pEntity->GetTilePos();
	CTile* pTile = m_parent->GetTile(tilePos);
	bool bTileRemoved = false;
	if (pTile == nullptr)
	{
		pTile = m_parent->GetTile(pEntity->GetPosition());
	}

	if (pTile != nullptr)
		bTileRemoved = pTile->RemovePlayer(pEntity);

	bool bGridRemoved = RemovePlayer(pEntity);

	if ((bGridRemoved || bTileRemoved) && pTile != nullptr)
		SendRemoveAOITile(pTile->GetCoord(), pEntity);
	else
		g_LogGame.ELog("ERROR LeaveGrid Entity:%d", pEntity->GetID());
}

bool CGrid::OnTransferGrid(CEntity* pEntity)
{
	if (pEntity->GetEntityType() == eENTITY_TYPE::ENTITY_PLAYER &&
		static_cast<CPlayer*>(pEntity)->GetRelease())
		return true;

	CTile* pTile = m_parent->GetTile(pEntity->GetPosition());
	if (pTile == nullptr || pTile->GetManagementGrid() != m_iID)
		return false;

	if (!AddPlayer(pEntity))
		return false;

	if (!pTile->AddPlayer(pEntity))
	{
		RemovePlayer(pEntity);
		return false;
	}

	// 이동 중인 Entity는 대상 Grid의 Move Vector까지 함께 인계한다.
	if (pEntity->GetMoveState() != eMOVESTATE::STOPPED)
		AddMoveVector(pEntity);

	return true;
}

void CGrid::OnTransferRollback(CEntity* pEntity, const COORDINATE& sourceTile)
{
	if (pEntity->GetEntityType() == eENTITY_TYPE::ENTITY_PLAYER &&
		static_cast<CPlayer*>(pEntity)->GetRelease())
		return;

	CTile* pTile = m_parent->GetTile(sourceTile);
	if (pTile == nullptr || pTile->GetManagementGrid() != m_iID)
		return;

	bool bGridAdded = AddPlayer(pEntity);
	bool bTileAdded = bGridAdded && pTile->AddPlayer(pEntity);
	if (!bTileAdded)
	{
		if (bGridAdded)
			RemovePlayer(pEntity);
		g_LogGame.ELog("ERROR Transfer Rollback Entity:%d", pEntity->GetID());
		return;
	}

	// 대상 Grid 등록 실패 시 원본 Grid의 관리 상태와 Move Vector를 복원한다.
	if (pEntity->GetMoveState() != eMOVESTATE::STOPPED)
		AddMoveVector(pEntity);
}

void CGrid::Init(int id, CMainWorld* pParent)
{
	m_iID = id;
	m_parent = pParent;
}

void CGrid::OnRegisterTile(CTile* pTile)
{
	m_vecTiles.push_back(pTile);
	pTile->OnReigsterGrid(m_iID);
}

void CGrid::EnqueueProcJob(PROC_MSG& msg)
{
	m_queueProc.Enqueue(msg);
}

void CGrid::PushEntityJob(int type, CEntity* pEntity, int sourceGridID, const COORDINATE& sourceTile)
{
	pEntity->AddQueRef();
	m_queueEntity.Push({ type, pEntity, sourceGridID, sourceTile });
}

void CGrid::EnqueueEntityJob(
	int type, CEntity* pEntity, int sourceGridID, const COORDINATE& sourceTile)
{
	if (pEntity == nullptr)
		return;

	PushEntityJob(type, pEntity, sourceGridID, sourceTile);
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

	// 등록 Tile 개수는 vector 자체를 기준으로 순회해 중복 카운트 상태를 제거한다.
	for (CTile* pTile : m_vecTiles)
		pTile->Update();
}

bool CGrid::AddPlayer(CEntity* pEntity)
{
	if (!m_vecPlayer.AddEntity(pEntity))
		return false;

	// Grid 컨테이너 등록 성공 후에만 외부에서 조회하는 소유 GridID를 갱신한다.
	pEntity->SetGridID(m_iID);
	return true;
}

bool CGrid::RemovePlayer(CEntity* pEntity)
{
	if (!m_vecPlayer.RemoveEntity(pEntity))
		return false;
	
	m_vecMove.RemoveEntity(pEntity);
	// 소유 Grid 제거 후 패킷 라우팅이 이전 Grid로 되돌아가지 않도록 ID를 비운다.
	pEntity->SetGridID(-1);

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
