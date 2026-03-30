#pragma once
#include "../CPlayer.h"
#include <vector>
#include <unordered_map>
#include <atomic>

#include "../MemoryManager/CLockFreeQueue_FromGPT.h"
#include "../GameServerDef.h"
#include  "../Zone/ZoneDefines.h"

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

	st_ZoneBounds m_ZoneBounds;				// Zone 의 Bounds 정보
	std::vector<st_Portal> m_vecPortal;		// Zone 의 Portal 정보
	std::vector<st_SpawnPoint> m_vecSpawnPoint;	// Zone 의 SpawnPoint 정보
	std::vector<st_TriggerVolume> m_vecTriggerVolume;	// Zone 의 TriggerVolume 정보
	std::unordered_map<int, std::vector<st_TriggerVolumeParam>> m_mapTriggerVolumeParams;	// <TriggerId, TriggerVolumeParam Vector>

	std::vector<CPlayer*> m_vecPlayer;				// Zone 이 관리하고 있는 Player
	std::unordered_map<int, int> m_mapIDtoIndex;	// <PlayerHandle,m_vecPlayerIndex>

	CLockFreeQueue_MPSC<ZONE_JOB> m_queue;
public:
	void InitZone(const st_ZoneBounds& bounds) { m_ZoneBounds = bounds; }
	void InsertPortal(st_Portal portal) { m_vecPortal.push_back(portal); }
	void InsertSpawnPoint(st_SpawnPoint spawn) { m_vecSpawnPoint.push_back(spawn); }
	void InsertTriggerVolume(st_TriggerVolume trigger) { m_vecTriggerVolume.push_back(trigger); }
	void InsertTriggerVolumeParam(int index, std::vector<st_TriggerVolumeParam> param) { m_mapTriggerVolumeParams[index] = param; }

public:
	void ZoneMoveJobProcess();
	void EnqueueJob(ZONE_JOB&& job) { m_queue.Enqueue(job); }

	bool PushTemp(CPlayer* pPlayer);

	virtual bool EnterZone(CPlayer* pPlayer);
	virtual bool LeaveZone(CPlayer* pPlayer);
	virtual bool TryEnterZone();
	int GetPid() { return m_ZonePid; }
};
