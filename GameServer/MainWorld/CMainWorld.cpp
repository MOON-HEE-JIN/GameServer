#include "CMainWorld.h"

#include "../NetWork/CNetServer.h"
#include "../Log/CLog.h"
#include "../CUtill/CUtill.h"

// MainWorld(1024, 1024)를 X/Y 방향 4 * 4 Grid로 나눈다.
// 각 Grid(256, 256)는 4 * 4 Tile(64, 64)을 관리한다.

CMainWorld::CMainWorld(int channel, int zoneid, int procid, int maxnum)
	:CZoneBasic(channel, zoneid, procid, maxnum)
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
	const int gridCount = m_GridCount[0] * m_GridCount[1];
	m_vecGrids.resize(gridCount);
	for (int i = 0; i < gridCount; i++)
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

			// Tile의 X/Y 위치로 관리 Grid를 구하고 행 우선 ID로 변환한다.
			const int gridX = (W * m_GridCount[0]) / m_iTileCountW;
			const int gridY = (H * m_GridCount[1]) / m_iTileCountH;
			const int GridID = gridY * m_GridCount[0] + gridX;
			m_vecGrids[GridID]->OnRegisterTile(m_vecTiles[tileID].get());
		}
	}

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

	st_Vector3F spawnPos = GetSpawnPos();
	if (spawnPos == st_Vector3F{ 0, 0, 0 })
	{
		g_LogGame.ELog("ERROR SpawnPoint 0 0 0, Temp Pos 32 0 32");
		spawnPos = st_Vector3F{ 32, 0, 32 };
	}
	pPlayer->StopMovement();
	pPlayer->SetPosition(spawnPos);

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
	pCGrid->EnqueueEntityJob(
		EGRID_MSG_TYPE::GRID_MSG_LEAVE, pPlayer, GridID, pPlayer->GetTilePos());

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

	g_LogGame.DLog("REQ Teleport Pos [%f, %f, %f]  NewTile [%d, %d]", pos.X, pos.Y, pos.Z, pNewTile->GetCoord().X, pNewTile->GetCoord().Z);

	CGrid* pCurGrid = GetGrid(pPlayer->GetGridID());
	CGrid* pNewGrid = GetGrid(pNewTile->GetManagementGrid());
	if (pCurGrid == nullptr || pNewGrid == nullptr)
		return false;

	int sourceGridID = pCurGrid->GetID();
	COORDINATE sourceTile = pPlayer->GetTilePos();
	st_Vector3F sourcePosition = pPlayer->GetPosition();
	pPlayer->MoveStop(sourcePosition);
	pPlayer->BeginGridTransfer(pNewGrid->GetID());

	// 대상 Grid 등록 완료 전에는 성공 응답을 보내지 않는다.
	if (!pCurGrid->RemoveForTeleport(pPlayer))
	{
		pPlayer->CompleteGridTransfer();
		return false;
	}

	pPlayer->SetPosition(pos);
	pNewGrid->EnqueueEntityJob(
		EGRID_MSG_TYPE::GRID_MSG_TELEPORT, pPlayer, sourceGridID, sourceTile, sourcePosition);

	return true;
}

bool CMainWorld::PushMoveVector(CEntity* pEntity)
{
	CGrid* pCurGrid = GetGrid(pEntity->GetGridID());
	
	if (pCurGrid == nullptr)
		return false;

	if (!pCurGrid->AddMoveVector(pEntity))
		return false;
	
	g_LogGame.DLog("Push MoveVector EntityID : %d, GridID : %d", pEntity->GetID(), pEntity->GetGridID());
	return true;
}

void CMainWorld::PopMoveVector(CEntity* pEntity)
{
	CGrid* pCurGrid = GetGrid(pEntity->GetGridID());

	if (pCurGrid == nullptr)
		return;

	pCurGrid->RemoveMoveVector(pEntity);

	g_LogGame.DLog("Pop MoveVector EntityID : %d, GridID : %d", pEntity->GetID(), pEntity->GetGridID());
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
	int ret = 0;
	
	double accumulatedtime = 0.0f;
	double lasttime = CUtil::GetQPCNowTime();

	while (m_bActive)
	{
		ret = WaitForSingleObject(m_hExit, 1);

		if (ret == WAIT_OBJECT_0)
			break;
	
		for (int i = 0; i < Loop; i++)
			vec[i]->ProcessPacket();

		// 지연 시간 누적
		double nowtime = CUtil::GetQPCNowTime();
		double frame = nowtime - lasttime;
		lasttime = nowtime;

		accumulatedtime += frame;

		int nLoop = 0;
		while (accumulatedtime >= FIXED_DELTA && nLoop < MAX_FRAME_LOOP_COUNT)
		{
			for (int i = 0; i < Loop; i++)
				vec[i]->Update();

			accumulatedtime -= FIXED_DELTA;
			nLoop++;
		}

		// 너무 많은 frame 이 쌓인다면 쌓인 frame 처리하느라 더 지연 최대 frame 만큼만 돌리기
		if (nLoop == MAX_FRAME_LOOP_COUNT)
		{
			accumulatedtime = 0.0f;
		}
	}
}

void CMainWorld::Start()
{
	m_bActive = true;

	int runid = 0;
	for (int i = 0; i < static_cast<int>(m_vecGrids.size()); i++)
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
	if (!IsValidGridID(id))
		return nullptr;

	return m_vecGrids[id].get();
}

