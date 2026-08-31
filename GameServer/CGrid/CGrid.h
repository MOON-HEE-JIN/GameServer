#pragma once
#include "../Stub/ProjectDefineStruct.h"
#include "../MemoryManager/CLockFreeQueue_SPSC.h"
#include "../NetWork/NetWorkDefine.h"
#include "../CUtill/CLockQueueh.h"
#include "../GameServerEnumDef.h"
#include "../CUtill/CEntityManagmentVector.h"
#include <cstdint>
#include <deque>
#include <unordered_map>
#include <unordered_set>

class CMainWorld;
class CTile;

struct st_GridJob
{
	int type;
	CEntity* pEntity;
	int SourceGridID;
	COORDINATE SourceTile;
	st_Vector3F SourcePosition;
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

	struct st_AoiTransition
	{
		CEntity* pEntity;
		COORDINATE PreviousTile;
		COORDINATE CurrentTile;
	};

	CMainWorld* m_parent;

	int m_iID;
	int m_iRunID;
	CLockFreeQueue_SPSC<PROC_MSG> m_queueProc;
	// 전환 직전 기존 Grid에 들어온 패킷은 다른 Grid Worker가 재전달할 수 있다.
	CLQueue<PROC_MSG> m_queueReroutedProc;
	CLQueue<st_GridJob> m_queueEntity;
	std::deque<PROC_MSG> m_deferredReroutedProc;
	std::deque<PROC_MSG> m_deferredProc;
	
	std::vector<CTile*> m_vecTiles;

	// Grid/Move 컨테이너의 Management Ref가 해당 Thread의 Entity 소유권을 표현한다.
	CEntityVector m_vecPlayer;
	CEntityVector m_vecMove;
	std::vector<CEntity*> m_vecCompleteMove;
	std::vector<st_GridTransfer> m_vecGridTransfer;
	std::vector<st_AoiTransition> m_vecAoiTransitions;
	std::vector<CEntity*> m_vecAoiPlayerBuffer;
	std::vector<CEntity*> m_vecPreviousOnlyPlayers;
	std::vector<CEntity*> m_vecCurrentOnlyPlayers;
	std::unordered_map<int, CEntity*> m_mapPreviousAoiPlayers;
	std::unordered_map<int, CEntity*> m_mapCurrentAoiPlayers;
#ifdef __DEBUG__
	uint64_t m_iLastDebugAoiRevision = 0;
	std::unordered_set<int> m_setExpectedAoiIDs;
#endif

	void EntityMoveRun();
	void EntityJobRun();
	void AddAoiTransition(CEntity* pEntity,
		const COORDINATE& previousTile, const COORDINATE& currentTile);
	void BuildAoiPlayerMap(const COORDINATE& pivot, bool previous,
		std::unordered_map<int, CEntity*>& players);
	void ProcessAoiTransitions();
	void ProcessProcJob(PROC_MSG& job, bool rerouted);
	void RerouteProcJob(PROC_MSG& job);

	void OnSpawnGrid(CEntity* pEntity);
	bool OnEnterGrid(CEntity* pEntity);
	bool OnLeaveGrid(CEntity* pEntity, const COORDINATE& sourceTile, bool addAoiTransition = true);
	bool OnTransferGrid(CEntity* pEntity, const COORDINATE& sourceTile);
	void OnTransferRollback(CEntity* pEntity, const COORDINATE& sourceTile);
	void OnTeleportRollback(CEntity* pEntity, const COORDINATE& sourceTile,
		const st_Vector3F& sourcePosition);
	void PushEntityJob(int type, CEntity* pEntity, int sourceGridID,
		const COORDINATE& sourceTile, const st_Vector3F& sourcePosition);

	bool AddPlayer(CEntity* pEntity);
	bool RemovePlayer(CEntity* pEntity);

public:
	void Init(int id, CMainWorld* pParent);
	void OnRegisterTile(CTile* pTile);

	int GetRunID() { return m_iRunID; }
	int GetID() const { return m_iID; }
	void SetRunID(int value) { m_iRunID = value; };

	bool RemoveForTeleport(CEntity* pEntity) { return OnLeaveGrid(pEntity, pEntity->GetTilePos(), false); };

	void EnqueueProcJob(PROC_MSG& msg);
	void EnqueueEntityJob(int type, CEntity* pEntity, int sourceGridID = -1,
		const COORDINATE& sourceTile = COORDINATE(-1, -1),
		const st_Vector3F& sourcePosition = st_Vector3F{});

	bool AddMoveVector(CEntity* pEntity);
	void RemoveMoveVector(CEntity* pEntity);

	void ProcessPacket();
	// 모든 Grid의 Entity 이동/소속 변경이 끝난 뒤 Tile AOI 작업을 실행한다.
	void UpdateEntity();
	void UpdateTransfer();
	void UpdateTile();
	void FinalizeAoiTick();
#ifdef __DEBUG__
	// 패킷 전송 없이 현재 Tile 구성과 Player 가시 목록의 무결성만 검사한다.
	void DebugCheckAOI();
#endif

};
