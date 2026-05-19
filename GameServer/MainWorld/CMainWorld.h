#pragma once

#include "../CGrid/CGrid.h"
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <vector>

#include "../MemoryManager/CLockFreeQueue_FromGPT.h"
#include "../NetWork/NetWorkDefine.h"
#include "../Zone/CZoneBasic.h"

#define MAX_MAINWORLD_THREAD_COUNT 4

struct st_ThreadParam
{
	int id;
	void* ptr;
};

class CMainWorld : public CZoneBasic
{
public:
	CMainWorld(int channel, int zoneid, int procid, int maxnum);
	~CMainWorld();

private:
	std::string m_strName = "MainWorld";
	
	st_ThreadParam params[MAX_MAINWORLD_THREAD_COUNT];
	HANDLE m_vecThreads[MAX_MAINWORLD_THREAD_COUNT];
	std::vector<std::vector<CGrid*>> m_vecThreadGrids;
	HANDLE m_hExit;

	//CLockFreeQueue_MPSC<PROC_MSG> m_ProcJobQueue[MAX_MAINWORLD_THREAD_COUNT];
private:
	int m_iGridWidth;
	int m_iGridHeight;
	CGrid* m_Grids;

	static unsigned __stdcall WorkerThread(void* arg);
protected:
	virtual void OnEnterZone(CPlayer* pPlayer) override;
	virtual void OnLeaveZone(CPlayer* pPlayer) override;

public:
	void MessageRouting(std::vector<PROC_MSG>& vec);
	virtual bool Teleport(CPlayer* pPlayer, st_Vector3F pos) override;
	virtual void Process() override {};
	virtual void PushMoveVector(CEntity* pEntity) override;
	void Run(int id);
	void Start();

public:
	bool IsValidCoord(COORDINATE& coord);
	bool IsValid(int x, int y);
	COORDINATE CalCoord(st_Vector3F pos);
public:
	CGrid* GetGrid(int x, int y);
};

