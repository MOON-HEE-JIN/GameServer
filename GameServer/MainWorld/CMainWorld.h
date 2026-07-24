#pragma once

#include "../CGrid/CGrid.h"
#include "../CGrid/CTile.h"

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <vector>

#include "../MemoryManager/CLockFreeQueue_FromGPT.h"
#include "../NetWork/NetWorkDefine.h"
#include "../Zone/CZoneBasic.h"

#define MAX_MAINWORLD_THREAD_COUNT 4
#define MAX_MANAGENTMENT_GRID_COUNT 4
#define DEFAULT_TILE_SIZE 64

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
	
	std::vector<std::vector<CGrid*>> m_vecThreadRunGrids;
	HANDLE m_hExit;

	//CLockFreeQueue_MPSC<PROC_MSG> m_ProcJobQueue[MAX_MAINWORLD_THREAD_COUNT];
private:
	int m_iTileCountW;
	int m_iTileCountH;
	int m_iAllTileCount;

	std::vector<CGrid*> m_vecGrids;
	std::vector<CTile*> m_vecTiles;

	static unsigned __stdcall WorkerThread(void* arg);
protected:
	virtual void OnEnterZone(CPlayer* pPlayer) override;
	virtual void OnLeaveZone(CPlayer* pPlayer) override;

public:
	void MessageRouting(std::vector<PROC_MSG>& vec);
	virtual bool Teleport(CPlayer* pPlayer, st_Vector3F pos) override;
	virtual void Process() override {};
	virtual void PushMoveVector(CEntity* pEntity) override;
	virtual bool SendZoneInfo(CPlayer* pPlayer) override;
	void Run(int id);
	void Start();

public:
	COORDINATE CalCoord(st_Vector3F pos);
public:
	int GetTileCountW() { return m_iTileCountW; }
	int GetTileCountH() { return m_iTileCountH; }
	int GetTileKey(COORDINATE& coord) { return coord.X * m_iTileCountW + coord.Z; }

	CTile* GetTile(st_Vector3F pos);
	CTile* GetTile(const COORDINATE& coord);
	CGrid* GetGrid(int id);

	bool IsValidGridID(int id) { return (id >= 0 && id < MAX_MANAGENTMENT_GRID_COUNT); }
};

