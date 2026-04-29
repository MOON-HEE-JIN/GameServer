#include "CZone_Login.h"

#include "../NetWork/CNetServer.h"
CZone_Login::CZone_Login(int ID, int ZoneID, int ProcID, int Maximum)
	: CZoneBase(ID, ZoneID, ProcID, Maximum)
{
	InitializeCriticalSection(&cs);
}

CZone_Login::~CZone_Login()
{
	DeleteCriticalSection(&cs);
}

Crit::Crit(CRITICAL_SECTION* _cs)
{
	cs = _cs;
	EnterCriticalSection(cs);
}

Crit::~Crit()
{
	LeaveCriticalSection(cs);
}
