#include "CMainWorld.h"

#include "../NetWork/CNetServer.h"
#include "../Stub/EnumDef.h"
#include "../Log/CLog.h"

CMainWorld::CMainWorld(int channel, int zoneid, int procid, int maxnum)
	:CZoneBasic(channel, zoneid, procid, maxnum)
{
	m_bMainWorld = true;

	m_iWidth = 1024;
	m_iHeight = 1024;

	m_iGridWidth = m_iWidth / MAX_MAINWORLD_THREAD_COUNT;		// 1024 / 4 == 256
	m_iGridHeight = m_iHeight / MAX_MAINWORLD_THREAD_COUNT;		// 1024 / 4 == 256

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

	m_Grids = new CGrid[MAX_MAINWORLD_THREAD_COUNT * MAX_MAINWORLD_THREAD_COUNT];

	m_hExit = CreateEvent(NULL, TRUE, FALSE, NULL);

	int x = 0, z = 0;
	for (int i = 0; i < MAX_MAINWORLD_THREAD_COUNT * MAX_MAINWORLD_THREAD_COUNT; i++)
	{
		m_Grids[i].Init(m_iWidth, m_iHeight, m_iGridWidth, m_iGridHeight, {(float)m_iGridWidth * x,0, (float)m_iGridHeight * z});
		if (++x >= MAX_MAINWORLD_THREAD_COUNT)
		{
			x = 0;
			z++;
		}
	}

	m_vecThreadGrids.resize(MAX_MAINWORLD_THREAD_COUNT);
	for (int i = 0; i < MAX_MAINWORLD_THREAD_COUNT; i++)
	{
		m_vecThreadGrids[i].resize(MAX_MAINWORLD_THREAD_COUNT);
	}
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
	CGrid* pCGrid = GetGrid(pPlayer->GetGridPos().X, pPlayer->GetGridPos().Z);
	if (pCGrid == nullptr)
	{
		g_LogGame.ELog("ERROR Enter Main World X, Z : [%d,%d]", pPlayer->GetGridPos().X, pPlayer->GetGridPos().Z);
		return;
	}

	if (!pCGrid->EnqueueAddPlayer(EGRID_ADD_TYPE::GRID_ENTER, pPlayer->GetID(), pPlayer))
	{
		g_LogGame.ELog("ERROR AddPlayer Tile  Pos: [%d,%d]", pPlayer->GetGridPos().X, pPlayer->GetGridPos().Z);
		return;
	}

	//g_LogGame.ILog("Enter %s World Channel : %d, ID : %d, Proc : %d ", m_strName.c_str(), GetChannel(), GetZoneID(), GetProcID());
}

void CMainWorld::OnLeaveZone(CPlayer* pPlayer)
{
	CGrid* pCGrid = GetGrid(pPlayer->GetGridPos().X, pPlayer->GetGridPos().Z);
	if (pCGrid == nullptr)
	{
		g_LogGame.ELog("ERROR Leave Main World X, Z : [%d,%d]", pPlayer->GetGridPos().X, pPlayer->GetGridPos().Z);
		return;
	}

	if (!pCGrid->EnqueueRemovePlayer(EGRID_ADD_TYPE::GRID_LEAVE, pPlayer->GetID(), pPlayer))
	{
		g_LogGame.ELog("ERROR SubPlayer Tile  Pos: [%d,%d]", pPlayer->GetGridPos().X, pPlayer->GetGridPos().Z);
		return;
	}

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

		const COORDINATE coord = pPlayer->GetGridPos();
		CGrid* pGrid = GetGrid(coord.X, coord.Z);
		if (pGrid == nullptr)
		{
			g_LogGame.ELog("ERROR MessageRouting [%d,%d]", coord.X, coord.Z);
			continue;
		}
		pGrid->Push(vec[i]);
	}
	
}

bool CMainWorld::Teleport(CPlayer* pPlayer, st_Vector3F pos)
{
	COORDINATE curCoord = pPlayer->GetGridPos();
	COORDINATE newCoord = CalCoord(pos);

	if (!IsValidCoord(newCoord))
		return false;

	CGrid* pCurGrid = GetGrid(curCoord.X, curCoord.Z);
	CGrid* pNewGrid = GetGrid(newCoord.X, newCoord.Z);

	// 같은 Thread 작업
	pCurGrid->RemovePlayer(pPlayer->GetID(), pPlayer);
	pPlayer->SetPosition(pos);

	if (curCoord.Z == newCoord.Z)
	{
		pNewGrid->AddPlayer(pPlayer->GetID(), pPlayer);

		st_STC_Teleport res;
		res.ret = 0;

		pPlayer->SendPacket(res);
	}
	else
	{
		pNewGrid->EnqueueAddPlayer(EGRID_ADD_TYPE::ADD_TELEPORT, pPlayer->GetID(), pPlayer);
	}
	
	return true;
}

void CMainWorld::PushMoveVector(CEntity* pEntity)
{
	COORDINATE curCoord = pEntity->GetGridPos();

	if (!IsValidCoord(curCoord))
		return ;

	CGrid* pCurGrid = GetGrid(curCoord.X, curCoord.Z);
	
	pCurGrid->AddMove(pEntity);
}

void CMainWorld::Run(int id)
{
	std::vector<CGrid*> vec = m_vecThreadGrids[id];
	int Loop = static_cast<int>(vec.size());
	int ret = 0;
	while (m_bActive)
	{
		ret = WaitForSingleObject(m_hExit, 1);

		for (int i = 0; i < Loop; i++)
			vec[i]->Update(this);
	}
}

void CMainWorld::Start()
{
	m_bActive = true;

	int x = 0;
	int y = 0;
	for (int i = 0; i < MAX_MAINWORLD_THREAD_COUNT * MAX_MAINWORLD_THREAD_COUNT; i++)
	{
		m_vecThreadGrids[y][x] = &m_Grids[i];
		if (++x >= MAX_MAINWORLD_THREAD_COUNT)
		{
			x = 0;
			y++;
		}
	}

	for (int i = 0; i < MAX_MAINWORLD_THREAD_COUNT; i++)
	{
		params[i].id = i;
		params[i].ptr = this;
		m_vecThreads[i] = ((HANDLE)_beginthreadex(NULL, 0, WorkerThread, &params[i], 0, NULL));
	}
}

bool CMainWorld::IsValidCoord(COORDINATE& coord)
{
	if (coord.X < 0 || coord.X > MAX_MAINWORLD_THREAD_COUNT || coord.Z < 0 || coord.Z > MAX_MAINWORLD_THREAD_COUNT)
		return false;
	return true;
}

bool CMainWorld::IsValid(int x, int y)
{
	if(x < 0 || x > m_iGridWidth || y < 0 || y > m_iGridHeight)
		return false;
	return true;
}

COORDINATE CMainWorld::CalCoord(st_Vector3F pos)
{
	return COORDINATE(static_cast<int>(pos.X / m_iGridWidth), static_cast<int>(pos.Z / m_iGridHeight));
}

CGrid* CMainWorld::GetGrid(int x, int y)
{
	if (x > m_iGridWidth || y > m_iGridHeight)
		return nullptr;

	return &m_Grids[y * m_iGridWidth + x];
}
