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

struct st_GridJob
{
	int type;
	int EntityID;
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
	CLQueue<st_GridJob> m_queueEntity;
	
	std::vector<CTile*> m_vecTiles;
	int m_iTileCount;

	// Grid 가 Player 을 관리할 필요가 있나??
	// 필요 없을거 같은데
	// 추후 확인후 삭제 해야함
	CEntityVector m_vecPlayer;
	CEntityVector m_vecMove;

	void EntityMoveRun();
	void EntityJobRun();

	void OnEnterGrid(int entityID);
	void OnLeaveGrid(int entityID);

	void OnTeleport(int entityID);

	bool AddPlayer(CEntity* pEntity);
	bool RemovePlayer(CEntity* pEntity);

	void SendInitAOITile(COORDINATE& pivot, CEntity* pEntity);
	void SendRemoveAOITile(COORDINATE& pivot, CEntity* pEntity);
public:
	void Init(int id, CMainWorld* pParent);
	void OnRegisterTile(CTile* pTile);

	int GetRunID() { return m_iRunID; }
	void SetRunID(int value) { m_iRunID = value; };

	void RemoveForTeleport(CEntity* pEntity) { OnLeaveGrid(pEntity->GetID()); };

	void EnqueueProcJob(PROC_MSG& msg);
	void EnqueueEntityJob(int type, int entityID);

	void AddMoveVector(CEntity* pEntity);
	void RemoveMoveVector(CEntity* pEntity);

	void Update();
};