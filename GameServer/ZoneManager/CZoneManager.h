#pragma once

#include "CZone.h"
#include <vector>
class CZoneManager
{
public:
	CZoneManager();
	~CZoneManager();
private:
	std::vector<CZone*> m_vecZone;
	int m_maxZoneCnt;

private:
	bool TryEnterZone(int toZone);

public:
	int GetProcID(int zone);
	int GetMaxZoneCnt(){ return m_maxZoneCnt; }
	
	bool ReqEnterZone(CPlayer* pPlayer, int toZone);
	void ReqJob(ZONE_JOB& job, int zone);
	int InitProcZoneVector(int pid, std::vector<CZone*>& vec);

	bool IsValidZoneID(int zoneid) const;
	bool IsEqualProcZoneID(int from, int to);

	bool EnterZone(CPlayer* pPlayer, int zoneid);
	bool LeaveZone(CPlayer* pPlayer);
public:
	void SendZone(int zone, CPacket* pPacket, CPlayer* pPlayer = nullptr);
	bool SendZoneInfo(int zone, CPlayer* pPlayer);
public:
	void Log();

};

extern CZoneManager g_ZoneManager;
