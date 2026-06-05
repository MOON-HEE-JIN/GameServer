#include "CMainWorld.h"

#include "../NetWork/CNetServer.h"
#include "../Stub/EnumDef.h"
#include "../Log/CLog.h"

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
	:CZoneBasic(channel, zoneid, procid, maxnum)
{
	m_bMainWorld = true;
	m_hExit = CreateEvent(NULL, TRUE, FALSE, NULL);

	m_iWidth = 1024;
	m_iHeight = 1024;

	m_iTileCountW = m_iWidth / DEFAULT_TILE_SIZE;
	m_iTileCountH = m_iHeight / DEFAULT_TILE_SIZE;

	m_iAllTileCount = m_iTileCountH * m_iTileCountW;

	// Tile 을 관리할 Grid
	m_Grids = new CGrid[MAX_MANAGENTMENT_GRID_COUNT];

	for (int i = 0; i < MAX_MANAGENTMENT_GRID_COUNT; i++)
		m_Grids[i].Init(this);

	// MainWorld Tile 로 나누기
	m_Tiles = new CTile[m_iAllTileCount];

	for (int H = 0; H < m_iTileCountH; H++)
	{
		for (int W = 0; W < m_iTileCountW; W++)
		{
			m_Tiles[H * m_iTileCountW + W].Init(
				{ W,H },
				{ static_cast<float>(W * DEFAULT_TILE_SIZE), 0, static_cast<float>(H * DEFAULT_TILE_SIZE) },
				{ static_cast<float>(W * DEFAULT_TILE_SIZE + DEFAULT_TILE_SIZE), 0, static_cast<float>(H * DEFAULT_TILE_SIZE + DEFAULT_TILE_SIZE) });
		
			m_Grids[H % MAX_MANAGENTMENT_GRID_COUNT].OnRegisterTile(&m_Tiles[H * m_iTileCountW + W]);
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
}

CMainWorld::~CMainWorld()
{
	m_bActive = false;

	for (int i = 0; i < MAX_MAINWORLD_THREAD_COUNT; i++)
	{
		SetEvent(m_hExit);
	}

	WaitForMultipleObjects(MAX_MAINWORLD_THREAD_COUNT, m_vecThreads, true, INFINITE);

	delete[] m_Grids;
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
	CTile* pTile = GetTile(pPlayer->GetPosition());
	CGrid* pCGrid = GetGrid(pTile->GetManagementGrid());

	if (pCGrid == nullptr)
	{
		g_LogGame.ELog("ERROR Enter Main World");
		return;
	}
	
	pCGrid->EnqueueEntityJob(EGRID_ADD_TYPE::GRID_ENTER, pPlayer->GetID(), pPlayer);
	pTile->AddPlayer(pPlayer);
	//g_LogGame.ILog("Enter %s World Channel : %d, ID : %d, Proc : %d ", m_strName.c_str(), GetChannel(), GetZoneID(), GetProcID());
}

void CMainWorld::OnLeaveZone(CPlayer* pPlayer)
{
	CTile* pTile = GetTile(pPlayer->GetPosition());
	CGrid* pCGrid = GetGrid(pTile->GetManagementGrid());

	if (pCGrid == nullptr)
	{
		g_LogGame.ELog("ERROR Leave Main World");
		return;
	}

	pCGrid->EnqueueEntityJob(EGRID_ADD_TYPE::GRID_LEAVE, pPlayer->GetID(), pPlayer);
	pTile->RemovePlayer(pPlayer);
	//g_LogGame.ILog("Leave %s World Channel : %d, ID : %d, Proc : %d ", m_strName.c_str(), GetChannel(), GetZoneID(), GetProcID());
}

void CMainWorld::MessageRouting(std::vector<PROC_MSG>& vec)
{	
	int Loop = static_cast<int>(vec.size());
	for (int i = 0; i < Loop; i++)
	{
		CPlayer* pPlayer = g_Net.GetPlayer(vec[i].PlayerHandle);
		if (pPlayer == nullptr)
			continue;

		CGrid* pGrid = GetGrid(pPlayer->GetGridID());
		if (pGrid == nullptr)
		{
			g_LogGame.ELog("ERROR MessageRouting");
			continue;
		}
		pGrid->EnqueueProcJob(vec[i]);
	}
	
}

bool CMainWorld::Teleport(CPlayer* pPlayer, st_Vector3F pos)
{
	CTile* pNewTile = GetTile(pos);
	
	g_LogGame.DLog("REQ Teleport Pos [%f, %f, %f]  NewTile [%d, %d]", pos.X, pos.Y, pos.Z, pNewTile->GetCoord().X, pNewTile->GetCoord().Z);

	if (pNewTile == nullptr)
		return false;

	CGrid* pCurGrid = GetGrid(pPlayer->GetGridID());
	CGrid* pNewGrid = GetGrid(pNewTile->GetManagementGrid());

	CTile* pCurTile = GetTile(pPlayer->GetPosition());
	pCurTile->RemovePlayer(pPlayer);
	
	if (pCurGrid->GetRunID() == pNewGrid->GetRunID())
	{
		g_LogGame.ELog("ERROR Teleport");
		return false;
	}

	pPlayer->SetPosition(pos);

	if (pCurGrid->GetRunID() == pNewGrid->GetRunID())
	{		
		pNewGrid->DirectAddPlayer(pPlayer);
		pNewTile->AddPlayer(pPlayer);
		return true;
	}
	
	// pCurGrid 여기서 Enqueue 로 할 이유가 있는가? 한번 생각 해보기
	pCurGrid->EnqueueEntityJob(EGRID_ADD_TYPE::GRID_LEAVE, pPlayer->GetID(), pPlayer);
	
	pPlayer->SetPosition(pos);
	pNewGrid->EnqueueEntityJob(EGRID_ADD_TYPE::ADD_TELEPORT, pPlayer->GetID(), pPlayer);
	
	return true;
}

void CMainWorld::PushMoveVector(CEntity* pEntity)
{
	CGrid* pCurGrid = GetGrid(pEntity->GetGridID());
	
	if (pCurGrid == nullptr)
		return;
	
	pCurGrid->AddMoveVector(pEntity);
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
	while (m_bActive)
	{
		ret = WaitForSingleObject(m_hExit, 1);

		for (int i = 0; i < Loop; i++)
			vec[i]->Update();
	}
}

void CMainWorld::Start()
{
	m_bActive = true;

	int runid = 0;
	for (int i = 0; i < MAX_MANAGENTMENT_GRID_COUNT; i++)
	{
		m_Grids[i].SetRunID(runid);
		m_vecThreadRunGrids[runid++].push_back(&m_Grids[i]);
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
	if (pos.X < 0 || pos.X > m_iWidth || pos.Z < 0 || pos.Z > m_iHeight)
		return nullptr;

	COORDINATE coord = {static_cast<int>(pos.X) / DEFAULT_TILE_SIZE, static_cast<int>(pos.Z) / DEFAULT_TILE_SIZE };
	
	return &m_Tiles[coord.Z * m_iTileCountW + coord.X];
}

CTile* CMainWorld::GetTile(const COORDINATE& coord)
{
	if (coord.Z < 0 || coord.Z >= m_iTileCountH || coord.X < 0 || coord.X >= m_iTileCountW)
		return nullptr;

	return &m_Tiles[coord.Z * m_iTileCountW + coord.X];
}

CGrid* CMainWorld::GetGrid(int id)
{
	if(id < 0 || id >= MAX_MANAGENTMENT_GRID_COUNT)
		return nullptr;

	return &m_Grids[id];
}

