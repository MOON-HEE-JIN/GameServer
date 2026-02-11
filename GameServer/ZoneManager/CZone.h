#pragma once
#include "../CPlayer.h"
#include <vector>
#include <unordered_map>
#include <atomic>
class CZone
{
public:
	CZone(int managerIndex, int pid, int max);
	~CZone();
	
	std::atomic<int> m_Cnt;
private:
	int m_ID;		// CZoneManager 에서 관리하는 ID;
	int m_ZonePid;	// 해당 Zone 을 관리해주는 ProcQ_ID;
	int m_MaxZoneManagerCount;		// Zone 에서 관리 하는 최대 Player 수

	std::vector<CPlayer*> m_vecPlayer; // Zone 이 관리하고 있는 Player
	std::unordered_map<int, int> m_mapIDtoIndex;	// <PlayerHandle,m_vecPlayerIndex>
public:
	bool EnterZone(CPlayer* pPlayer);
	bool LeaveZone(CPlayer* pPlayer);
	int GetPid() { return m_ZonePid; }
};