#include "CMainWorld.h"

#include "../NetWork/CNetServer.h"
#include "../Stub/EnumDef.h"
#include "../Log/CLog.h"
#include "../CUtill/CUtill.h"
// 
	// MainWorld 당 MAX_MAINWORLD_THREAD_COUNT == 4 Thread 담당
	// MainWorld 를 MAX_MAINWORLD_THREAD_COUNT * MAX_MAINWORLD_THREAD_COUNT 로 나눈다
	// 
	// Grid 는 총 4 * 4 == 16
	// Thread 는 Grid[h] 를 담당 총 4개의 Grid 에 Update 담당
	// 
	// 다시 Grid 는 임시(4 * 4) 만큼의 Tile 을 보유
	// 1:MainWolrd(1024, 1024) -> 16:Grid(256, 256) -> 16:Tile(64, 64)
	//
	// 
	// 다른 방법 생각
	// MainWorld 를 Grid 로 나누는것이 아닌 Tile 로 나눈다
	// 그후 Grid 에 관리 Tile 을 넣어 준다
	// 기존 에는 MainWorld -> Grid -> Tile 을 나눴지만
	// MainWorld->Tile , Grid 에 Tile 등록 으로 바꾼다면?
	// 기존 방식은 이동식 tile 이동 에 있어 계산을 할려면 Grid 를 먼저 계산해야한다
	// 그렇다면 아예 최소단위는 통으로 가지고 있고 관리를 Grid 로 한다면??
	//

CMainWorld::CMainWorld(int channel, int zoneid, int procid, int maxnum)
	:CZoneBasic(channel, zoneid, procid, maxnum),
	m_UpdateBarrier(MAX_MAINWORLD_THREAD_COUNT)
{
	m_bMainWorld = true;
	m_hExit = CreateEvent(NULL, TRUE, FALSE, NULL);
	for (int i = 0; i < MAX_MAINWORLD_THREAD_COUNT; i++)
		m_vecThreads[i] = NULL;

	m_iWidth = 1024;
	m_iHeight = 1024;

	m_iTileCountW = m_iWidth / DEFAULT_TILE_SIZE;
	m_iTileCountH = m_iHeight / DEFAULT_TILE_SIZE;

	m_iAllTileCount = m_iTileCountH * m_iTileCountW;

	// Tile 을 관리할 Grid
	m_vecGrids.resize(MAX_MANAGENTMENT_GRID_COUNT);
	for (int i = 0; i < MAX_MANAGENTMENT_GRID_COUNT; i++)
	{
		m_vecGrids[i] = std::make_unique<CGrid>();
		m_vecGrids[i]->Init(i, this);
	}

	// MainWorld Tile 로 나누기
	m_vecTiles.resize(m_iAllTileCount);

	for (int H = 0; H < m_iTileCountH; H++)
	{
		for (int W = 0; W < m_iTileCountW; W++)
		{
			int tileID = H * m_iTileCountW + W;
			m_vecTiles[tileID] = std::make_unique<CTile>();
			m_vecTiles[tileID]->Init(this, { W,H });

			// 인접한 Z Tile을 연속 Grid 대역에 배치해 Thread 간 Grid 전환 횟수를 줄인다.
			int GridID = (H * MAX_MANAGENTMENT_GRID_COUNT) / m_iTileCountH;
			m_vecGrids[GridID]->OnRegisterTile(m_vecTiles[tileID].get());
		}
	}

	// 일정 크기로 Grid 에 tile 을 등록해주는 방식이라면
	// 그러면 Grid 의 bounds 0 ~ 512  512 ~ 1024 이렇게 2 * 2 로 나누고
	// Grid bounds 에 해당하는 tile 을 넣어준다면
	// 지금 방식은 어느 타일이든 Grid 에 들어갈수 있고 Grid 에서 관리하는 Tile 의 규칙이 없다면
	// 서로 다른 Grid 사이에서의 이동이 많이 일어날까  범위를 맞춘 Grid 하고 차이가 날까
	// 범위 에 맞게 tile 이 설정된 Grid vs 임의 tile 을 가지고 있는 Grid
	// 어느 쪽이 서로 다른 Thread 사이의 이동이 많이 일어나는가? --> 맵의 구성요소 에 따라 달라지지 않나?
	// 일단 진행
	//

	m_vecThreadRunGrids.resize(MAX_MAINWORLD_THREAD_COUNT);

	// 임시 Spawn 위치
	m_stSpawnPos = { 32, 0, 32 };
}

CMainWorld::~CMainWorld()
{
	m_bActive = false;

	SetEvent(m_hExit);
	for (int i = 0; i < MAX_MAINWORLD_THREAD_COUNT; i++)
	{
		if (m_vecThreads[i] != NULL)
		{
			WaitForSingleObject(m_vecThreads[i], INFINITE);
			CloseHandle(m_vecThreads[i]);
			m_vecThreads[i] = NULL;
		}
	}

	if (m_hExit != NULL)
	{
		CloseHandle(m_hExit);
		m_hExit = NULL;
	}
}

unsigned __stdcall CMainWorld::WorkerThread(void* arg)
{
	st_ThreadParam* param = (st_ThreadParam*)arg;
	CMainWorld* p = (CMainWorld*)param->ptr;
	
	p->Run(param->id);

	return 0;
}

void CMainWorld::OnEnterZone(CPlayer* pPlayer)
{
	if (pPlayer == nullptr)
		return;

	// Grid/Tile는 Spawn 위치를 기준으로 등록되어야 한다.
	pPlayer->SetPosition(GetSpawnPos());
	pPlayer->MoveStop(pPlayer->GetPosition());

	CTile* pTile = GetTile(pPlayer->GetPosition());
	if (pTile == nullptr)
	{
		g_LogGame.ELog("ERROR Enter Main World Invalid Position");
		return;
	}

	int GridID = pTile->GetManagementGrid();

	if (!IsValidGridID(GridID))
	{
		g_LogGame.ELog("ERROR Enter Main World");
		return;
	}
	CGrid* pCGrid = GetGrid(GridID);
	// 최초 TilePos는 Grid 등록 과정에서 확정되므로 위치로 계산한 Grid에 Spawn을 위임한다.
	pCGrid->EnqueueEntityJob(EGRID_MSG_TYPE::GRID_MSG_SPAWN, pPlayer);
	
	//g_LogGame.ILog("Enter %s World Channel : %d, ID : %d, Proc : %d ", m_strName.c_str(), GetChannel(), GetZoneID(), GetProcID());
}

void CMainWorld::OnLeaveZone(CPlayer* pPlayer)
{
	if (pPlayer == nullptr)
		return;

	// 위치 변경 중에도 실제 소유 Grid가 제거하도록 저장된 GridID를 우선 사용한다.
	int GridID = pPlayer->GetGridID();
	if (!IsValidGridID(GridID))
	{
		CTile* pOwnedTile = GetTile(pPlayer->GetTilePos());
		if (pOwnedTile != nullptr)
			GridID = pOwnedTile->GetManagementGrid();
	}

	if (!IsValidGridID(GridID))
	{
		g_LogGame.ELog("ERROR Leave Main World Invalid Grid");
		return;
	}

	CGrid* pCGrid = GetGrid(GridID);
	const COORDINATE sourceTile = pPlayer->GetTilePos();
	pCGrid->EnqueueEntityJob(EGRID_MSG_TYPE::GRID_MSG_LEAVE, pPlayer, GridID, sourceTile);

	//g_LogGame.ILog("Leave %s World Channel : %d, ID : %d, Proc : %d ", m_strName.c_str(), GetChannel(), GetZoneID(), GetProcID());
}

void CMainWorld::MessageRouting(std::vector<PROC_MSG>& vec)
{	
	int Loop = static_cast<int>(vec.size());
	for (int i = 0; i < Loop; i++)
	{
		CPlayer* pPlayer = g_Net.GetPlayer(vec[i].PlayerHandle, vec[i].SessionHandle);
		if (pPlayer == nullptr)
			continue;

		int RoutingGridID = pPlayer->GetRoutingGridID();
		CGrid* pGrid = GetGrid(RoutingGridID);
		if (pGrid == nullptr)
		{
			g_LogGame.ELog("ERROR MessageRouting Player:%d RouteGrid:%d OwnerGrid:%d",
				pPlayer->GetID(), RoutingGridID, pPlayer->GetGridID());
			continue;
		}
		pGrid->EnqueueProcJob(vec[i]);
	}
	
}

bool CMainWorld::Teleport(CPlayer* pPlayer, st_Vector3F pos)
{
	// 추후 Teleport 관련 조건 체크
	if (0)
	{
		g_LogGame.ELog("ERROR Teleport");
		return false;
	}

	CTile* pNewTile = GetTile(pos);

	if (pNewTile == nullptr)
		return false;

	//g_LogGame.DLog("REQ Teleport Pos [%f, %f, %f]  NewTile [%d, %d]", pos.X, pos.Y, pos.Z, pNewTile->GetCoord().X, pNewTile->GetCoord().Z);

	CGrid* pCurGrid = GetGrid(pPlayer->GetGridID());
	CGrid* pNewGrid = GetGrid(pNewTile->GetManagementGrid());
	if (pCurGrid == nullptr || pNewGrid == nullptr)
		return false;

	const int sourceGridID = pCurGrid->GetID();
	const COORDINATE sourceTile = pPlayer->GetTilePos();
	const st_Vector3F sourcePosition = pPlayer->GetPosition();
	pPlayer->MoveStop(sourcePosition);
	pPlayer->BeginGridTransfer(pNewGrid->GetID());

	// 순서를 위해서 먼저 삭제 후 이동
	if (!pCurGrid->RemoveForTeleport(pPlayer))
	{
		pPlayer->CompleteGridTransfer();
		return false;
	}
	
	pPlayer->SetPosition(pos);

	pNewGrid->EnqueueEntityJob(EGRID_MSG_TYPE::GRID_MSG_TELEPORT, pPlayer,
		sourceGridID, sourceTile, sourcePosition);

	return true;
}

bool CMainWorld::PushMoveVector(CEntity* pEntity)
{
	CGrid* pCurGrid = GetGrid(pEntity->GetGridID());
	
	if (pCurGrid == nullptr)
		return false;

	if (!pCurGrid->AddMoveVector(pEntity))
		return false;
	
	//g_LogGame.DLog("Push MoveVector EntityID : %d, GridID : %d", pEntity->GetID(), pEntity->GetGridID());
	return true;
}

void CMainWorld::PopMoveVector(CEntity* pEntity)
{
	CGrid* pCurGrid = GetGrid(pEntity->GetGridID());

	if (pCurGrid == nullptr)
		return;

	pCurGrid->RemoveMoveVector(pEntity);

	//g_LogGame.DLog("Pop MoveVector EntityID : %d, GridID : %d", pEntity->GetID(), pEntity->GetGridID());
}

void CMainWorld::BoradCast(CPacket* pPacket, COORDINATE pivot, CPlayer* pPlayer)
{
	// 해당 Player 에게 해당 Grid 에 있는 Player 들의 정보 보내기
	for (int z = -AOI_VIEW_COUNT; z <= AOI_VIEW_COUNT; z++)
	{
		for (int x = -AOI_VIEW_COUNT; x <= AOI_VIEW_COUNT; x++)
		{
			CTile* pAOITile = GetTile({ pivot.X + x, pivot.Z + z });
			if (pAOITile == nullptr)
				continue;

			if (pAOITile->GetActiveCount() == 0)
				continue;

			pAOITile->EnqueueBroadCast(pPlayer, pPacket);
		}
	}
}

bool CMainWorld::SendZoneInfo(CPlayer* pPlayer)
{
	// tile 기반 주변 정보 보내기
	return false;
}

void CMainWorld::Run(int id)
{
	std::vector<CGrid*> vec = m_vecThreadRunGrids[id];
	int Loop = static_cast<int>(vec.size());
	uint64_t completedTick = 0;
	double accumulatedtime = 0.0f;
	double lasttime = CUtil::GetQPCNowTime();

	while (true)
	{
		const bool stopping = WaitForSingleObject(m_hExit, 1) == WAIT_OBJECT_0;
		if (!stopping)
		{
			for (int i = 0; i < Loop; i++)
				vec[i]->ProcessPacket();
		}

		// Thread 0만 공통 Fixed Tick을 발행해 Worker별 시간 오차를 제거한다.
		if (id == 0)
		{
			if (m_bActive.load(std::memory_order_acquire))
			{
				double nowtime = CUtil::GetQPCNowTime();
				accumulatedtime += nowtime - lasttime;
				lasttime = nowtime;

				int updateCount = 0;
				while (accumulatedtime >= FIXED_DELTA && updateCount < MAX_FRAME_LOOP_COUNT)
				{
					accumulatedtime -= FIXED_DELTA;
					++updateCount;
				}

				if (updateCount > 0)
					m_iPublishedUpdateTick.fetch_add(updateCount, std::memory_order_release);

				if (updateCount == MAX_FRAME_LOOP_COUNT)
					accumulatedtime = 0.0f;
			}
			else
			{
				// 종료 결정은 한 Worker만 발행해 마지막 Tick 도중 일부 Thread만 빠지는 것을 막는다.
				m_bWorkersExit.store(true, std::memory_order_release);
			}
		}

		const uint64_t targetTick = m_iPublishedUpdateTick.load(std::memory_order_acquire);
		while (completedTick < targetTick)
		{
			for (int i = 0; i < Loop; i++)
				vec[i]->UpdateEntity();

			// 일반 이동에서 발생한 다른 Grid 인계 작업을 같은 Tick 안에 확정한다.
			m_UpdateBarrier.arrive_and_wait();
			for (int i = 0; i < Loop; i++)
				vec[i]->UpdateTransfer();

			// 모든 Grid/Tile 소속 변경이 끝난 안정된 상태에서 AOI 차이만 계산한다.
			m_UpdateBarrier.arrive_and_wait();
			for (int i = 0; i < Loop; i++)
				vec[i]->UpdateTile();

			// 다른 Grid의 이전 Tile 스냅샷 참조가 모두 끝날 때까지 보존한다.
			m_UpdateBarrier.arrive_and_wait();
#ifdef __DEBUG__
			for (int i = 0; i < Loop; i++)
				vec[i]->DebugCheckAOI();

			// Debug 검사는 인접 Grid의 현재 Tile을 읽으므로 다음 Tick과 분리한다.
			m_UpdateBarrier.arrive_and_wait();
#endif
			for (int i = 0; i < Loop; i++)
				vec[i]->FinalizeAoiTick();

			++completedTick;
		}

		if (m_bWorkersExit.load(std::memory_order_acquire) &&
			completedTick >= m_iPublishedUpdateTick.load(std::memory_order_acquire))
			break;
	}
}

void CMainWorld::Start()
{
	m_bActive = true;
	m_bWorkersExit.store(false, std::memory_order_release);
	m_iPublishedUpdateTick.store(0, std::memory_order_release);

	int runid = 0;
	for (int i = 0; i < MAX_MANAGENTMENT_GRID_COUNT; i++)
	{
		m_vecGrids[i]->SetRunID(runid);
		m_vecThreadRunGrids[runid++].push_back(m_vecGrids[i].get());
		if (runid >= MAX_MAINWORLD_THREAD_COUNT)
			runid = 0;
	}

	for (int i = 0; i < MAX_MAINWORLD_THREAD_COUNT; i++)
	{
		params[i].id = i;
		params[i].ptr = this;
		m_vecThreads[i] = ((HANDLE)_beginthreadex(NULL, 0, WorkerThread, &params[i], 0, NULL));
	}
}

COORDINATE CMainWorld::CalCoord(st_Vector3F pos)
{
	return { static_cast<int>(pos.X) / DEFAULT_TILE_SIZE, static_cast<int>(pos.Z) / DEFAULT_TILE_SIZE };
}

CTile* CMainWorld::GetTile(st_Vector3F pos)
{
	// 월드 최대 좌표는 마지막 Tile의 다음 경계이므로 범위에서 제외한다.
	if (pos.X < 0 || pos.X >= m_iWidth || pos.Z < 0 || pos.Z >= m_iHeight)
		return nullptr;

	COORDINATE coord = {static_cast<int>(pos.X) / DEFAULT_TILE_SIZE, static_cast<int>(pos.Z) / DEFAULT_TILE_SIZE };

	return m_vecTiles[coord.Z * m_iTileCountW + coord.X].get();
}

CTile* CMainWorld::GetTile(const COORDINATE& coord)
{
	if (coord.Z < 0 || coord.Z >= m_iTileCountH || coord.X < 0 || coord.X >= m_iTileCountW)
		return nullptr;

	return m_vecTiles[coord.Z * m_iTileCountW + coord.X].get();
}

CGrid* CMainWorld::GetGrid(int id)
{
	if(id < 0 || id >= MAX_MANAGENTMENT_GRID_COUNT)
		return nullptr;

	return m_vecGrids[id].get();
}

