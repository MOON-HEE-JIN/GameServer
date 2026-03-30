#pragma once
#include "../CPlayer.h"
#include <vector>
#include <unordered_map>
#include <atomic>

#include "../MemoryManager/CLockFreeQueue_FromGPT.h"
#include "../GameServerDef.h"

class CZone
{
public:
	CZone(int managerIndex, int pid, int max);
	~CZone();
	
	std::atomic<int> m_Cnt;
protected:
	int m_ID;		// CZoneManager 에서 관리하는 ID;
	int m_ZonePid;	// 해당 Zone 을 관리해주는 ProcQ_ID;
	int m_MaxZoneManagerCount;		// Zone 에서 관리 하는 최대 Player 수

	std::vector<CPlayer*> m_vecPlayer;				// Zone 이 관리하고 있는 Player
	std::unordered_map<int, int> m_mapIDtoIndex;	// <PlayerHandle,m_vecPlayerIndex>

	CLockFreeQueue_MPSC<ZONE_JOB> m_queue;

	std::vector<CEntity*> m_vecEntityMoveVector;			// 움직임 전용 Update Vector
public:
	void ZoneMoveJobProcess();		// Zone 이동 Job 처리
	void ZoneEntityMoveProcess();	// Entity Position 이동 처리
public:
	void EnqueueJob(ZONE_JOB&& job) { m_queue.Enqueue(job); }

	bool PushTemp(CPlayer* pPlayer);

	void PushMoveVector(CEntity* pEntity);
	void PopMoveVector(CEntity* pEntity);

	bool SendZoneInfo(CPlayer* pPlayer);
	void SendBroadCast(CPacket* pPacket, CPlayer* pPlayer = nullptr);
public:
	virtual bool EnterZone(CPlayer* pPlayer);
	virtual bool LeaveZone(CPlayer* pPlayer);
	virtual bool TryEnterZone();
public:
	int GetPid() { return m_ZonePid; }
};
