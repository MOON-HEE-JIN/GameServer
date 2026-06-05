#pragma once
#include "../Stub/ProjectDefineStruct.h"
#include "../MemoryManager/CLockFreeQueue_SPSC.h"
#include "../NetWork/NetWorkDefine.h"
#include "../CUtill/CLockQueueh.h"
#include "../GameServerEnumDef.h"
#include "../CUtill/CEntityManagmentVector.h"

#include <unordered_map>
#include <set>

class CMainWorld;
class CTile;

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
	CMainWorld* m_parent;

	int m_iID;
	int m_iRunID;
	CLockFreeQueue_SPSC<PROC_MSG> m_queueProc;
	CLQueue<st_AddMsg> m_queueEntity;
	
	std::set<int> m_setTileKeys;
	std::vector<CTile*> m_vecTiles;
	int m_iTileCount;

	// Grid 가 Player 을 관리할 필요가 있나??
	// 필요 없을거 같은데
	// 추후 확인후 삭제 해야함
	CEntityVector m_vecPlayer{ EIndexType::VECTOR_INDEX_GRID };
	CEntityVector m_vecMove{ EIndexType::VECTOR_INDEX_MOVE };

	void EntityMoveRun();
	void EntityJobRun();

	void OnTeleport(CEntity* pEntity);

	bool AddPlayer(CEntity* pEntity);
	bool RemovePlayer(CEntity* pEntity);
public:
	void Init(CMainWorld* pParent);
	void OnRegisterTile(CTile* pTile);

	int GetRunID() { return m_iRunID; }
	void SetRunID(int value) { m_iRunID = value; };

	bool DirectAddPlayer(CEntity* pEntity) { return AddPlayer(pEntity); }
	bool DirectRemovePlayer(CEntity* pEntity) { return RemovePlayer(pEntity); }

	void EnqueueProcJob(PROC_MSG& msg);
	void EnqueueEntityJob(int type, int key, CEntity* pEntity);

	void AddMoveVector(CEntity* pEntity);
	void RemoveMoveVector(CEntity* pEntity);

	void Update();
};