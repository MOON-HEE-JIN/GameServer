#pragma once
#include "../Stub/ProjectDefineStruct.h"
#include "../MemoryManager/CLockFreeQueue_SPSC.h"
#include "../NetWork/NetWorkDefine.h"
#include "../CUtill/CLockQueueh.h"
#include "../GameServerEnumDef.h"
#include "../CUtill/CEntityManagmentVector.h"

class CMainWorld;
class CTile;

struct st_GridJob
{
	int type;
	CEntity* pEntity;
	int SourceGridID;
	COORDINATE SourceTile;
};

class CGrid
{
public:
	CGrid();
	~CGrid();

private:
	struct st_GridTransfer
	{
		CEntity* pEntity;
		CGrid* pNewGrid;
		CTile* pSourceTile;
		COORDINATE SourceTile;
	};

	CMainWorld* m_parent;

	int m_iID;
	int m_iRunID;
	CLockFreeQueue_SPSC<PROC_MSG> m_queueProc;
	CLQueue<st_GridJob> m_queueEntity;
	
	std::vector<CTile*> m_vecTiles;

	// Grid/Move 컨테이너의 Management Ref가 해당 Thread의 Entity 소유권을 표현한다.
	CEntityVector m_vecPlayer;
	CEntityVector m_vecMove;
	std::vector<CEntity*> m_vecCompleteMove;
	std::vector<st_GridTransfer> m_vecGridTransfer;

	void EntityMoveRun();
	void EntityJobRun();

	void OnSpawnGrid(CEntity* pEntity);
	bool OnEnterGrid(CEntity* pEntity);
	void OnLeaveGrid(CEntity* pEntity);
	bool OnTransferGrid(CEntity* pEntity);
	void OnTransferRollback(CEntity* pEntity, const COORDINATE& sourceTile);
	void PushEntityJob(int type, CEntity* pEntity, int sourceGridID, const COORDINATE& sourceTile);

	bool AddPlayer(CEntity* pEntity);
	bool RemovePlayer(CEntity* pEntity);

public:
	void Init(int id, CMainWorld* pParent);
	void OnRegisterTile(CTile* pTile);

	int GetRunID() { return m_iRunID; }
	void SetRunID(int value) { m_iRunID = value; };

	void RemoveForTeleport(CEntity* pEntity) { OnLeaveGrid(pEntity); };

	void EnqueueProcJob(PROC_MSG& msg);
	void EnqueueEntityJob(int type, CEntity* pEntity, int sourceGridID = -1,
		const COORDINATE& sourceTile = COORDINATE(-1, -1));

	bool AddMoveVector(CEntity* pEntity);
	void RemoveMoveVector(CEntity* pEntity);

	void ProcessPacket();
	void Update();
};
