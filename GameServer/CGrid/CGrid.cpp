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
		st_STC_AoiOutPlayer outAoiPlayer{};
		outAoiPlayer.ID = vec[i]->GetID();
		CPacket outAoiPacket;
		outAoiPacket << outAoiPlayer;

		// 범위 안으로 들어옴
		st_STC_AoiInPlayer inAoiPlayer{};
		inAoiPlayer.info.ID = vec[i]->GetID();
		inAoiPlayer.info.pos = vec[i]->GetPosition();
		inAoiPlayer.info.speed = vec[i]->GetMoveSpeed();

		CPacket inAoiPacket;
		inAoiPacket << inAoiPlayer;

		if (vec[i]->GetMoveState() != eMOVESTATE::STOPPED)
		{
			st_STC_OtherMoveStart inAoiPlayerMove{};
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
				MinOutW++;

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
				MinInW++;
			else if (diff.X > 0)
				MaxInW--;

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

		// 실제 Grid 등록이 잠시 비더라도 패킷은 대상 Grid로 계속 라우팅한다.
		// 대상 Grid는 TRANSFER_IN 완료 전 패킷을 deferred queue에 보관한다.
		transfer.pEntity->BeginGridTransfer(transfer.pNewGrid->GetID());

		if (!transfer.pSourceTile->RemovePlayer(transfer.pEntity))
		{
			transfer.pEntity->CompleteGridTransfer();
			continue;
		}
		if (!RemovePlayer(transfer.pEntity))
		{
			transfer.pSourceTile->AddPlayer(transfer.pEntity);
			transfer.pEntity->CompleteGridTransfer();
			continue;
		}

		transfer.pNewGrid->PushEntityJob(
			EGRID_MSG_TYPE::GRID_MSG_TRANSFER_IN, transfer.pEntity, m_iID,
			transfer.SourceTile, st_Vector3F{});
	}
}

void CGrid::EntityJobRun()
{
	// Jobs deferred during a Grid handoff keep their existing queue reference.
	int deferredLoop = static_cast<int>(m_deferredEntity.size());
	for (int i = 0; i < deferredLoop; i++)
	{
		m_queueEntity.Push(m_deferredEntity.front());
		m_deferredEntity.pop_front();
	}

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
			int routingGridID = pEntity->GetRoutingGridID();
			if (routingGridID != m_iID)
			{
				CGrid* pRoutingGrid = m_parent->GetGrid(routingGridID);
				if (pRoutingGrid != nullptr)
				{
					pRoutingGrid->EnqueueEntityJob(
						EGRID_MSG_TYPE::GRID_MSG_LEAVE, pEntity,
						routingGridID, pEntity->GetTilePos());
				}
				else if (!(pEntity->GetEntityType() == eENTITY_TYPE::ENTITY_PLAYER &&
					static_cast<CPlayer*>(pEntity)->GetRelease() && pEntity->GetGridID() < 0))
				{
					g_LogGame.ELog("ERROR Leave Route Entity:%d Route:%d Grid:%d",
						pEntity->GetID(), routingGridID, pEntity->GetGridID());
					if (pEntity->GetEntityType() == eENTITY_TYPE::ENTITY_PLAYER)
						g_Net.PlayerDisConnect(static_cast<CPlayer*>(pEntity)->GetSessionHandle());
				}
				break;
			}

			// A transfer job may already be queued but not registered in this Grid yet.
			if (pEntity->GetGridID() != m_iID)
			{
				m_deferredEntity.push_back(msg);
				continue;
			}

			pEntity->StopMovement();
			if (!OnLeaveGrid(pEntity, pEntity->GetTilePos()) &&
				pEntity->GetEntityType() == eENTITY_TYPE::ENTITY_PLAYER)
			{
				g_Net.PlayerDisConnect(static_cast<CPlayer*>(pEntity)->GetSessionHandle());
			}
		}
			break;
		case EGRID_MSG_TYPE::GRID_MSG_TRANSFER_IN:
		{
			if (!OnTransferGrid(pEntity))
			{
				CGrid* pSourceGrid = m_parent->GetGrid(msg.SourceGridID);
				if (pSourceGrid != nullptr)
				{
					// Rollback 완료 전에도 패킷은 원본 Grid에서 유실 없이 대기한다.
					pEntity->BeginGridTransfer(msg.SourceGridID);
					pSourceGrid->EnqueueEntityJob(
						EGRID_MSG_TYPE::GRID_MSG_TRANSFER_ROLLBACK,
						pEntity, msg.SourceGridID, msg.SourceTile);
				}
				else
				{
					pEntity->CompleteGridTransfer();
					g_LogGame.ELog("ERROR Transfer Source Grid Entity:%d Grid:%d",
						pEntity->GetID(), msg.SourceGridID);
					if (pEntity->GetEntityType() == eENTITY_TYPE::ENTITY_PLAYER)
						g_Net.PlayerDisConnect(static_cast<CPlayer*>(pEntity)->GetSessionHandle());
				}
			}
		}
			break;
		case EGRID_MSG_TYPE::GRID_MSG_TRANSFER_ROLLBACK:
		{
			OnTransferRollback(pEntity, msg.SourceTile);
		}
			break;
		case EGRID_MSG_TYPE::GRID_MSG_TELEPORT:
		{
			if (pEntity->GetEntityType() == eENTITY_TYPE::ENTITY_PLAYER &&
				static_cast<CPlayer*>(pEntity)->GetRelease())
			{
				pEntity->CompleteGridTransfer();
				break;
			}

			if (OnEnterGrid(pEntity))
			{
				// 등록 직후 연결 종료와 경합한 경우 대상 Grid에 고아 Entity를 남기지 않는다.
				if (pEntity->GetEntityType() == eENTITY_TYPE::ENTITY_PLAYER &&
					static_cast<CPlayer*>(pEntity)->GetRelease())
				{
					OnLeaveGrid(pEntity, pEntity->GetTilePos());
					pEntity->CompleteGridTransfer();
					break;
				}

				pEntity->CompleteGridTransfer();
				if (pEntity->GetEntityType() == eENTITY_TYPE::ENTITY_PLAYER)
				{
					st_STC_Teleport res{};
					res.ret = ERROR_CODE::NOT_ERROR;
					res.pos = pEntity->GetPosition();
					static_cast<CPlayer*>(pEntity)->SendPacket(res);
				}
			}
			else
			{
				CGrid* pSourceGrid = m_parent->GetGrid(msg.SourceGridID);
				if (pSourceGrid != nullptr)
				{
					pEntity->SetPosition(msg.SourcePosition);
					pEntity->BeginGridTransfer(msg.SourceGridID);
					pSourceGrid->EnqueueEntityJob(
						EGRID_MSG_TYPE::GRID_MSG_TELEPORT_ROLLBACK,
						pEntity, msg.SourceGridID, msg.SourceTile, msg.SourcePosition);
				}
				else
				{
					pEntity->CompleteGridTransfer();
					g_LogGame.ELog("ERROR Teleport Source Grid Entity:%d Grid:%d",
						pEntity->GetID(), msg.SourceGridID);
					if (pEntity->GetEntityType() == eENTITY_TYPE::ENTITY_PLAYER)
						g_Net.PlayerDisConnect(static_cast<CPlayer*>(pEntity)->GetSessionHandle());
				}
			}
		}
			break;
		case EGRID_MSG_TYPE::GRID_MSG_TELEPORT_ROLLBACK:
		{
			OnTeleportRollback(pEntity, msg.SourceTile, msg.SourcePosition);
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
	if (pEntity->GetEntityType() == eENTITY_TYPE::ENTITY_PLAYER &&
		static_cast<CPlayer*>(pEntity)->GetRelease())
		return;

	// Grid와 Tile 등록이 모두 성공한 경우에만 Spawn 완료 상태를 공개한다.
	if (!OnEnterGrid(pEntity))
	{
		if (pEntity->GetEntityType() == eENTITY_TYPE::ENTITY_PLAYER)
			g_Net.PlayerDisConnect(static_cast<CPlayer*>(pEntity)->GetSessionHandle());
		return;
	}

	if (pEntity->GetEntityType() == eENTITY_TYPE::ENTITY_PLAYER)
	{
		CPlayer* pPlayer = (CPlayer*)pEntity;
		// 새로운 Zone 에 입장 완료
		pEntity->SetZoneStatus(eZONESTATUS::STABLE);
		{
			st_STC_ChangeZone pack{};
			pack.ret = 0;
			pack.channel = pPlayer->GetChannel();
			pack.zone = pPlayer->GetZoneID();
			pack.spawn = pPlayer->GetPosition();
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

bool CGrid::OnLeaveGrid(CEntity* pEntity, const COORDINATE& sourceTile)
{
	// 비동기 작업에 저장한 Tile을 사용해 실제 소유 Tile에서 제거한다.
	COORDINATE tilePos = sourceTile;
	if (tilePos.X < 0 || tilePos.Z < 0)
		tilePos = pEntity->GetTilePos();
	CTile* pTile = m_parent->GetTile(tilePos);
	if (pTile == nullptr)
		return false;

	bool bTileRemoved = pTile->RemovePlayer(pEntity);
	bool bGridRemoved = RemovePlayer(pEntity);

	if (!bTileRemoved || !bGridRemoved)
	{
		// 부분 제거로 Entity가 고아가 되지 않도록 원래 컨테이너를 복원한다.
		if (bGridRemoved)
			AddPlayer(pEntity);
		if (bTileRemoved)
			pTile->AddPlayer(pEntity);
		g_LogGame.ELog("ERROR LeaveGrid Entity:%d Tile:%d Grid:%d",
			pEntity->GetID(), bTileRemoved, bGridRemoved);
		return false;
	}

	SendRemoveAOITile(pTile->GetCoord(), pEntity);
	return true;
}

bool CGrid::OnTransferGrid(CEntity* pEntity)
{
	if (pEntity->GetEntityType() == eENTITY_TYPE::ENTITY_PLAYER &&
		static_cast<CPlayer*>(pEntity)->GetRelease())
	{
		pEntity->CompleteGridTransfer();
		return true;
	}

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

	if (pEntity->GetEntityType() == eENTITY_TYPE::ENTITY_PLAYER &&
		static_cast<CPlayer*>(pEntity)->GetRelease())
	{
		OnLeaveGrid(pEntity, pEntity->GetTilePos());
		pEntity->CompleteGridTransfer();
		return true;
	}

	pEntity->CompleteGridTransfer();
	return true;
}

void CGrid::OnTransferRollback(CEntity* pEntity, const COORDINATE& sourceTile)
{
	if (pEntity->GetEntityType() == eENTITY_TYPE::ENTITY_PLAYER &&
		static_cast<CPlayer*>(pEntity)->GetRelease())
	{
		pEntity->CompleteGridTransfer();
		return;
	}

	CTile* pTile = m_parent->GetTile(sourceTile);
	if (pTile == nullptr || pTile->GetManagementGrid() != m_iID)
	{
		pEntity->CompleteGridTransfer();
		if (pEntity->GetEntityType() == eENTITY_TYPE::ENTITY_PLAYER)
			g_Net.PlayerDisConnect(static_cast<CPlayer*>(pEntity)->GetSessionHandle());
		return;
	}

	bool bGridAdded = AddPlayer(pEntity);
	bool bTileAdded = bGridAdded && pTile->AddPlayer(pEntity);
	if (!bTileAdded)
	{
		if (bGridAdded)
			RemovePlayer(pEntity);
		g_LogGame.ELog("ERROR Transfer Rollback Entity:%d", pEntity->GetID());
		pEntity->CompleteGridTransfer();
		if (pEntity->GetEntityType() == eENTITY_TYPE::ENTITY_PLAYER)
			g_Net.PlayerDisConnect(static_cast<CPlayer*>(pEntity)->GetSessionHandle());
		return;
	}

	// 대상 Grid 등록 실패 시 원본 Grid의 관리 상태와 Move Vector를 복원한다.
	if (pEntity->GetMoveState() != eMOVESTATE::STOPPED)
		AddMoveVector(pEntity);

	pEntity->CompleteGridTransfer();
}

void CGrid::OnTeleportRollback(CEntity* pEntity, const COORDINATE& sourceTile,
	const st_Vector3F& sourcePosition)
{
	if (pEntity->GetEntityType() == eENTITY_TYPE::ENTITY_PLAYER &&
		static_cast<CPlayer*>(pEntity)->GetRelease())
	{
		pEntity->CompleteGridTransfer();
		return;
	}

	pEntity->SetPosition(sourcePosition);
	pEntity->StopMovement();
	CTile* pTile = m_parent->GetTile(sourceTile);
	bool bGridAdded = pTile != nullptr && pTile->GetManagementGrid() == m_iID && AddPlayer(pEntity);
	bool bTileAdded = bGridAdded && pTile->AddPlayer(pEntity);
	if (!bTileAdded)
	{
		if (bGridAdded)
			RemovePlayer(pEntity);
		g_LogGame.ELog("ERROR Teleport Rollback Entity:%d", pEntity->GetID());
		if (pEntity->GetEntityType() == eENTITY_TYPE::ENTITY_PLAYER)
			g_Net.PlayerDisConnect(static_cast<CPlayer*>(pEntity)->GetSessionHandle());
	}
	else if (pEntity->GetEntityType() == eENTITY_TYPE::ENTITY_PLAYER &&
		static_cast<CPlayer*>(pEntity)->GetRelease())
	{
		OnLeaveGrid(pEntity, sourceTile);
	}

	pEntity->CompleteGridTransfer();
	if (pEntity->GetEntityType() == eENTITY_TYPE::ENTITY_PLAYER)
	{
		st_STC_Teleport res{};
		res.ret = ERROR_CODE::NOT_EQUAL_POSITION;
		res.pos = sourcePosition;
		static_cast<CPlayer*>(pEntity)->SendPacket(res);
	}
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

void CGrid::RerouteProcJob(PROC_MSG& job)
{
	m_queueReroutedProc.Push(job);
}

void CGrid::PushEntityJob(int type, CEntity* pEntity, int sourceGridID,
	const COORDINATE& sourceTile, const st_Vector3F& sourcePosition)
{
	pEntity->AddQueRef();
	m_queueEntity.Push({ type, pEntity, sourceGridID, sourceTile, sourcePosition });
}

void CGrid::EnqueueEntityJob(
	int type, CEntity* pEntity, int sourceGridID, const COORDINATE& sourceTile,
	const st_Vector3F& sourcePosition)
{
	if (pEntity == nullptr)
		return;

	PushEntityJob(type, pEntity, sourceGridID, sourceTile, sourcePosition);
}

bool CGrid::AddMoveVector(CEntity* pEntity)
{
	return m_vecMove.AddEntity(pEntity);
}

void CGrid::RemoveMoveVector(CEntity* pEntity)
{
	m_vecMove.RemoveEntity(pEntity);
}

void CGrid::ProcessProcJob(PROC_MSG& job, bool rerouted)
{
	CPlayer* pPlayer = g_Net.GetPlayer(job.PlayerHandle, job.SessionHandle);
	if (pPlayer == nullptr)
		return;
	if (pPlayer->GetZoneStatus() != eZONESTATUS::STABLE)
	{
		st_STC_ChangeingZone res;
		res.ret = ERROR_CODE::ZONE_CHANEING;
		res.type = job.type;

		pPlayer->SendPacket(res);
		return;
	}

	int RoutingGridID = pPlayer->GetRoutingGridID();
	if (RoutingGridID != m_iID)
	{
		CGrid* pRoutingGrid = m_parent->GetGrid(RoutingGridID);
		if (pRoutingGrid != nullptr)
		{
			pRoutingGrid->RerouteProcJob(job);
		}
		else
		{
			g_LogGame.ELog("ERROR MessageRerouting Player:%d RouteGrid:%d OwnerGrid:%d",
				pPlayer->GetID(), RoutingGridID, pPlayer->GetGridID());
		}
		return;
	}

	// 패킷 목적지는 이미 대상 Grid지만 실제 컨테이너 등록은 아직 진행 중이다.
	if (pPlayer->GetGridID() != m_iID)
	{
		if (rerouted)
			m_deferredReroutedProc.push_back(job);
		else
			m_deferredProc.push_back(job);
		return;
	}

	proc.DO_GAME_Proc(job.type, pPlayer, job.packet);
}

void CGrid::ProcessPacket()
{
	PROC_MSG job;

	// Route 변경 직전에 기존 Grid로 들어간 패킷이 새 Grid로 직접 라우팅된 패킷보다
	// 먼저 처리되도록 reroute/deferred queue를 분리한다.
	int ReroutedDeferredLoop = static_cast<int>(m_deferredReroutedProc.size());
	for (int i = 0; i < ReroutedDeferredLoop; i++)
	{
		job = std::move(m_deferredReroutedProc.front());
		m_deferredReroutedProc.pop_front();
		ProcessProcJob(job, true);
	}

	while (m_queueReroutedProc.POP(job))
		ProcessProcJob(job, true);

	int DeferredLoop = static_cast<int>(m_deferredProc.size());
	for (int i = 0; i < DeferredLoop; i++)
	{
		job = std::move(m_deferredProc.front());
		m_deferredProc.pop_front();
		ProcessProcJob(job, false);
	}

	while (m_queueProc.TryDequeue(job))
		ProcessProcJob(job, false);
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
	pEntity->ClearGridID(m_iID);

	return true;
}

void CGrid::SendInitAOITile(COORDINATE& pivot, CEntity* pEntity)
{
	st_STC_AoiInPlayer res{};
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
	st_STC_AoiOutPlayer res{};
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
