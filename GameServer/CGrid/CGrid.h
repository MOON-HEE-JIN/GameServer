#pragma once
#include "../Stub/ProjectDefineStruct.h"
#include "../MemoryManager/CLockFreeQueue_SPSC.h"
#include "../NetWork/NetWorkDefine.h"
#include "../CUtill/CLockQueueh.h"
#include "../GameServerEnumDef.h"
#include "../CUtill/CEntityManagmentVector.h"
#include "CTile.h"

#include <unordered_map>

struct st_AddMsg
{
	int type;
	int key;
	CEntity* pEntity;
};

class CGrid
{
public:
	CGrid();
	~CGrid();

private:
	int m_iID;
	CLockFreeQueue_SPSC<PROC_MSG> m_queueProc;
	CLQueue<st_AddMsg> m_queueEntity;

	std::vector<CTile*> m_vecTiles;
	int m_iTileCount;

	CEntityVector m_vecPlayer{ EVECTOR_INDEX_TYPE::GRID };
	CEntityVector m_vecMove{ EVECTOR_INDEX_TYPE::MOVE };

	void EntityJobRun();

	bool AddPlayer(CEntity* pEntity);
	bool RemovePlayer(CEntity* pEntity);
public:
	void OnRegisterTile(CTile* pTile);

	void EnqueueProcJob(PROC_MSG& msg);
	void EnqueueEntityJob(int type, int key, CEntity* pEntity);

	void AddMoveVector(CEntity* pEntity);
	void RemoveMoveVector(CEntity* pEntity);

	void Update();
};