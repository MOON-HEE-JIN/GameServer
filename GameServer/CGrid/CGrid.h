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
	CEntity* pEntity;
};

struct st_GridChange
{
	CEntity* pEntity;
	CGrid* pGrid;
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
	CEntityVector m_vecPlayer{ EIndexType::VECTOR_INDEX_GRID };
	CEntityVector m_vecMove{ EIndexType::VECTOR_INDEX_MOVE };
	std::vector<st_GridChange> m_vecChangeThreadMove;

	void EntityMoveRun();
	void EntityJobRun();

	void OnEnterZone(CEntity* pEntity);
	void OnLeaveZone(CEntity* pEntity);
	void OnTeleport(CEntity* pEntity);
	void OnChangeThread(CEntity* pEntity);

	bool AddPlayer(CEntity* pEntity);
	bool RemovePlayer(CEntity* pEntity);

	void SendInitAOITile(COORDINATE& pivot, CEntity* pEntity);
	void SendRemoveAOITile(COORDINATE& pivot, CEntity* pEntity);
public:
	void Init(int id, CMainWorld* pParent);
	void OnRegisterTile(CTile* pTile);

	int GetRunID() { return m_iRunID; }
	void SetRunID(int value) { m_iRunID = value; };

	bool DirectAddPlayer(CEntity* pEntity) { return AddPlayer(pEntity); }
	bool DirectRemovePlayer(CEntity* pEntity) { return RemovePlayer(pEntity); }

	void EnqueueProcJob(PROC_MSG& msg);
	void EnqueueEntityJob(int type, CEntity* pEntity);

	bool AddMoveVector(CEntity* pEntity);
	void RemoveMoveVector(CEntity* pEntity);

	void ProcessPacket();
	void Update();
};