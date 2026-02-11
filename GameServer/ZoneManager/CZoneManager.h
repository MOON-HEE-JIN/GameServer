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

public:
	int GetProcID(int zone);
	int GetMaxZoneCnt(){ return m_maxZoneCnt; }
	
	bool IsValidZoneID(int zoneid) const;
	bool EnterZone(CPlayer* pPlayer, int zoneid);
	bool LeaveZone(CPlayer* pPlayer);
	void Log();
};

extern CZoneManager g_ZoneManager;