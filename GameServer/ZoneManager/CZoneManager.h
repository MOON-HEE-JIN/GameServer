#pragma once

#include "CZone.h"
#include <vector>
class CZoneManager
{
public:
	CZoneManager();
	~CZoneManager();
private:
	bool IsValidZoneID(int zoneid) const;
	int GetProcID(int zone);
	int m_maxZoneCnt;
public:
	int GetMaxZoneCnt(){ return m_maxZoneCnt; }
	bool EnterZone(CPlayer* pPlayer, int zoneid);
	bool LeaveZone(CPlayer* pPlayer);
	int GetProcID(int zone) { return m_vecZone[zone]->GetPid(); }
	void Log();
};

extern CZoneManager g_ZoneManager;