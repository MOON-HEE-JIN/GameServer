#pragma once

#include "../CPlayer.h"
#include "../MemoryManager/CLockFreeQueue_FromGPT.h"
#include "../GameServerDef.h"

#include <vector>
#include <unordered_map>

class CZoneBase
{
public:
	CZoneBase(int ID, int ZoneID, int ProcID, int Maximum);
	~CZoneBase();
	
private:
	int m_iChannel;
	int m_iZoneID;
	int m_iProcID;
	int m_iMaximumUser;

	std::atomic<bool> m_bActive;
	std::atomic<int> m_iCurCnt;
protected:
	std::vector<CPlayer*> m_vecPlayers;
	
private:
	std::unordered_map<int, int> m_mapIDtoIndex;

	CLockFreeQueue_MPSC<ZONE_CHANGE_JOB> m_queue;

public:
	virtual void Init(int ID, int ZoneID, int ProcID, int Maximum);
	virtual void Reset();

	void ZoneChangeJobProcess();
	virtual void Process() = 0;
public:
	int GetChannel() { return m_iChannel; }
	int GetZoneID() { return m_iZoneID; }
	int GetProcID() { return m_iProcID; }
	int GetMaximum() { return m_iMaximumUser; }
	int GetCurCnt() { return m_iCurCnt.load(); }
public:
	bool Enqueue(ZONE_CHANGE_JOB& job);
	bool TryPush(CPlayer* pPlayer);
	virtual bool EnterZone(CPlayer* pPlayer);
	virtual bool LeaveZone(CPlayer* pPlayer);
	virtual void OnLeaveZone(CPlayer* pPlayer) {};
	bool TryEnterZone();

public:
	void SendBoradCast(CPacket* pPacket, CPlayer* pPlayer = nullptr);
	virtual bool SendZoneInfo(CPlayer* pPlayer) { return true; };
};