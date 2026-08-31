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
	m_vecAoiTransitions.reserve(32);
	m_vecAoiPlayerBuffer.reserve(128);
	m_vecPreviousOnlyPlayers.reserve(64);
	m_vecCurrentOnlyPlayers.reserve(64);
	m_mapPreviousAoiPlayers.reserve(128);
	m_mapCurrentAoiPlayers.reserve(128);
#ifdef __DEBUG__
	m_setExpectedAoiIDs.reserve(128);
#endif
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

#ifdef __DEBUG__
		COORDINATE diff = newTilePos - curTilePos;
		// 이동 으로 인한 tile 변경은 1 로 제한 된다
		if (abs(diff.X) > 1 || abs(diff.Z) > 1)
			g_LogGame.DLog("ERROR diff > 1");
#endif // __DEBUG__

		// 같은 Grid 에서 관리 할때
		if (pCurTile->GetManagementGrid() == pNewTile->GetManagementGrid())
		{
			// 같은 Grid의 Tile 변경은 현재 Thread에서 제거와 등록을 원자적인 순서로 처리한다.
			if (!pCurTile->RemovePlayer(vec[i]))
				continue;
			if (pNewTile->AddPlayer(vec[i]))
			{
				AddAoiTransition(vec[i], curTilePos, newTilePos);
			}
			else
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
			if (OnEnterGrid(pEntity))
				AddAoiTransition(pEntity, COORDINATE(-1, -1), pEntity->GetTilePos());
		}
			break;
		case EGRID_MSG_TYPE::GRID_MSG_LEAVE:
		{
			OnLeaveGrid(pEntity, msg.SourceTile);
		}
			break;
		case EGRID_MSG_TYPE::GRID_MSG_TRANSFER_IN:
		{
			if (!OnTransferGrid(pEntity, msg.SourceTile))
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
					g_LogGame.ELog("ERROR Transfer Source Grid Entity:%d Grid:%d",
						pEntity->GetID(), msg.SourceGridID);
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
			if (OnEnterGrid(pEntity))
			{
				AddAoiTransition(pEntity, msg.SourceTile, pEntity->GetTilePos());
				pEntity->CompleteGridTransfer();
				if (pEntity->GetEntityType() == eENTITY_TYPE::ENTITY_PLAYER)
				{
					st_STC_Teleport response{};
					response.ret = ERROR_CODE::NOT_ERROR;
					response.pos = pEntity->GetPosition();
					static_cast<CPlayer*>(pEntity)->SendPacket(response);
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
	if (pEntity->GetEntityType() == eENTITY_TYPE::ENTITY_PLAYER)
		static_cast<CPlayer*>(pEntity)->ResetVisiblePlayers();

	// Grid와 Tile 등록이 모두 성공한 경우에만 Spawn 완료 상태를 공개한다.
	if (!OnEnterGrid(pEntity))
		return;
	AddAoiTransition(pEntity, COORDINATE(-1, -1), pEntity->GetTilePos());

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
			st_Vector3F pos = pPlayer->GetPosition();
			pack.spawn = pos;

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

	return true;
}

bool CGrid::OnLeaveGrid(CEntity* pEntity, const COORDINATE& sourceTile, bool addAoiTransition)
{
	// Zone 전환 후 다른 World가 TilePos를 갱신해도 작업 등록 시점의 소유 Tile을 제거한다.
	COORDINATE tilePos = sourceTile;
	if (tilePos.X < 0 || tilePos.Z < 0)
		tilePos = pEntity->GetTilePos();
	CTile* pTile = m_parent->GetTile(tilePos);
	bool bTileRemoved = false;
	if (pTile == nullptr)
	{
		pTile = m_parent->GetTile(pEntity->GetPosition());
	}

	if (pTile != nullptr)
		bTileRemoved = pTile->RemovePlayer(pEntity);

	bool bGridRemoved = RemovePlayer(pEntity);

	if (bGridRemoved != bTileRemoved)
	{
		// 부분 제거 상태는 즉시 원래 컨테이너 구성으로 되돌린다.
		if (bGridRemoved)
			AddPlayer(pEntity);
		if (bTileRemoved && pTile != nullptr)
			pTile->AddPlayer(pEntity);
		g_LogGame.ELog("ERROR Partial LeaveGrid Entity:%d", pEntity->GetID());
		return false;
	}

	if (!bGridRemoved || pTile == nullptr)
	{
		g_LogGame.ELog("ERROR LeaveGrid Entity:%d", pEntity->GetID());
		return false;
	}
	if (addAoiTransition)
		AddAoiTransition(pEntity, tilePos, COORDINATE(-1, -1));

	return true;
}

bool CGrid::OnTransferGrid(CEntity* pEntity, const COORDINATE& sourceTile)
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

	AddAoiTransition(pEntity, sourceTile, pTile->GetCoord());
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

	pEntity->CompleteGridTransfer();
}

void CGrid::OnTeleportRollback(CEntity* pEntity, const COORDINATE& sourceTile,
	const st_Vector3F& sourcePosition)
{
	pEntity->SetPosition(sourcePosition);
	CTile* pTile = m_parent->GetTile(sourceTile);
	bool restored = pTile != nullptr && pTile->GetManagementGrid() == m_iID;
	if (restored)
	{
		const bool gridAdded = AddPlayer(pEntity);
		const bool tileAdded = gridAdded && pTile->AddPlayer(pEntity);
		restored = gridAdded && tileAdded;
		if (!restored && gridAdded)
			RemovePlayer(pEntity);
	}

	pEntity->CompleteGridTransfer();
	if (!restored)
		g_LogGame.ELog("ERROR Teleport Rollback Entity:%d", pEntity->GetID());

	if (pEntity->GetEntityType() == eENTITY_TYPE::ENTITY_PLAYER)
	{
		st_STC_Teleport response{};
		response.ret = ERROR_CODE::NOT_EQUAL_POSITION;
		response.pos = sourcePosition;
		static_cast<CPlayer*>(pEntity)->SendPacket(response);
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

void CGrid::UpdateEntity()
{
	EntityJobRun();
	EntityMoveRun();
}

void CGrid::UpdateTransfer()
{
	// EntityMoveRun에서 다른 Grid로 전달한 작업을 같은 Tick 안에 확정한다.
	EntityJobRun();
}

void CGrid::UpdateTile()
{
	// Tile 작업 후 이동한 Entity의 이전/현재 AOI 차이만 반영한다.
	for (CTile* pTile : m_vecTiles)
		pTile->Update();

	ProcessAoiTransitions();
}

void CGrid::FinalizeAoiTick()
{
	// 모든 Grid의 AOI 처리가 끝난 뒤에만 이전 Tick 스냅샷 참조를 반환한다.
	for (CTile* pTile : m_vecTiles)
		pTile->ClearAoiSnapshot();
}

void CGrid::AddAoiTransition(CEntity* pEntity,
	const COORDINATE& previousTile, const COORDINATE& currentTile)
{
	if (pEntity == nullptr || previousTile == currentTile)
		return;

	pEntity->AddQueRef();
	m_vecAoiTransitions.push_back({ pEntity, previousTile, currentTile });
}

void CGrid::BuildAoiPlayerMap(const COORDINATE& pivot, bool previous,
	std::unordered_map<int, CEntity*>& players)
{
	players.clear();
	m_vecAoiPlayerBuffer.clear();
	if (m_parent->GetTile(pivot) == nullptr)
		return;

	for (int z = -AOI_VIEW_COUNT; z <= AOI_VIEW_COUNT; ++z)
	{
		for (int x = -AOI_VIEW_COUNT; x <= AOI_VIEW_COUNT; ++x)
		{
			CTile* pTile = m_parent->GetTile({ pivot.X + x, pivot.Z + z });
			if (pTile == nullptr)
				continue;

			if (previous)
				pTile->AppendPreviousPlayers(m_vecAoiPlayerBuffer);
			else
				pTile->AppendCurrentPlayers(m_vecAoiPlayerBuffer);
		}
	}

	for (CEntity* pEntity : m_vecAoiPlayerBuffer)
	{
		if (pEntity == nullptr || pEntity->GetEntityType() != eENTITY_TYPE::ENTITY_PLAYER)
			continue;
		if (!previous && static_cast<CPlayer*>(pEntity)->GetRelease())
			continue;
		players.try_emplace(pEntity->GetID(), pEntity);
	}
}

void CGrid::ProcessAoiTransitions()
{
	for (const st_AoiTransition& transition : m_vecAoiTransitions)
	{
		CEntity* pEntity = transition.pEntity;
		if (pEntity == nullptr)
			continue;

		BuildAoiPlayerMap(transition.PreviousTile, true, m_mapPreviousAoiPlayers);
		BuildAoiPlayerMap(transition.CurrentTile, false, m_mapCurrentAoiPlayers);

		const int entityID = pEntity->GetID();
		m_mapPreviousAoiPlayers.erase(entityID);
		m_mapCurrentAoiPlayers.erase(entityID);
		m_vecPreviousOnlyPlayers.clear();
		m_vecCurrentOnlyPlayers.clear();

		for (const auto& [id, pPrevious] : m_mapPreviousAoiPlayers)
		{
			if (!m_mapCurrentAoiPlayers.contains(id))
				m_vecPreviousOnlyPlayers.push_back(pPrevious);
		}
		for (const auto& [id, pCurrent] : m_mapCurrentAoiPlayers)
		{
			if (!m_mapPreviousAoiPlayers.contains(id))
				m_vecCurrentOnlyPlayers.push_back(pCurrent);
		}

		if (pEntity->GetEntityType() == eENTITY_TYPE::ENTITY_PLAYER)
		{
			CPlayer* pPlayer = static_cast<CPlayer*>(pEntity);
			if (!pPlayer->GetRelease() && m_parent->GetTile(transition.CurrentTile) != nullptr)
				pPlayer->ApplyAoiDelta(m_vecPreviousOnlyPlayers, m_vecCurrentOnlyPlayers);
			else
				pPlayer->ResetVisiblePlayers();

			// 이동한 Player의 변경을 이전/현재 주변 Player에게 대칭으로 반영한다.
			for (CEntity* pPrevious : m_vecPreviousOnlyPlayers)
			{
				if (pPrevious->GetEntityType() != eENTITY_TYPE::ENTITY_PLAYER)
					continue;
				CPlayer* pRecipient = static_cast<CPlayer*>(pPrevious);
				if (!pRecipient->GetRelease())
					pRecipient->NotifyAoiLeave(entityID);
			}
			for (CEntity* pCurrent : m_vecCurrentOnlyPlayers)
			{
				if (pCurrent->GetEntityType() != eENTITY_TYPE::ENTITY_PLAYER)
					continue;
				CPlayer* pRecipient = static_cast<CPlayer*>(pCurrent);
				if (!pRecipient->GetRelease())
					pRecipient->NotifyAoiEnter(pEntity);
			}
		}

		pEntity->ReleaseQueRef();
	}
	m_vecAoiTransitions.clear();
}

#ifdef __DEBUG__
void CGrid::DebugCheckAOI()
{
	const uint64_t aoiRevision = m_parent->GetAoiRevision();
	if (m_iLastDebugAoiRevision == aoiRevision)
		return;

	// Debug 감시는 패킷을 보내지 않고 서버의 가시 목록 불일치만 기록한다.
	const std::vector<CEntity*>& players = m_vecPlayer.GetVector();
	for (CEntity* pEntity : players)
	{
		if (pEntity == nullptr || pEntity->GetEntityType() != eENTITY_TYPE::ENTITY_PLAYER)
			continue;

		CPlayer* pPlayer = static_cast<CPlayer*>(pEntity);
		if (pPlayer->GetRelease())
			continue;

		BuildAoiPlayerMap(pEntity->GetTilePos(), false, m_mapCurrentAoiPlayers);
		m_mapCurrentAoiPlayers.erase(pEntity->GetID());
		m_setExpectedAoiIDs.clear();
		for (const auto& [id, pVisible] : m_mapCurrentAoiPlayers)
		{
			if (pVisible != nullptr)
				m_setExpectedAoiIDs.insert(id);
		}

		int missingCount = 0;
		int staleCount = 0;
		if (!pPlayer->DebugCheckVisiblePlayers(
			m_setExpectedAoiIDs, missingCount, staleCount))
		{
			g_LogGame.ELog(
				"ERROR DebugCheckAOI Player:%d Grid:%d Tile:[%d,%d] Missing:%d Stale:%d",
				pPlayer->GetID(), m_iID,
				pPlayer->GetTilePos().X, pPlayer->GetTilePos().Z,
				missingCount, staleCount);
		}
	}

	m_iLastDebugAoiRevision = aoiRevision;
}
#endif

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
	// 다른 Grid가 이미 소유권을 인수했다면 새 GridID를 지우지 않는다.
	pEntity->ClearGridID(m_iID);

	return true;
}
