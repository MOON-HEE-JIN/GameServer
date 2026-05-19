#pragma once
#include "../Zone/CZoneBasic.h"

// windows.h 가 winsock.h 를 포함 하여 h 오류 발생
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

class Crit
{
public:
	Crit(CRITICAL_SECTION* _cs);
	~Crit();
	CRITICAL_SECTION* cs;
};

// 초기 GameServer 접속하고 Packet 을 처리 해주는 Login Zone
class CZone_Login : public CZoneBasic
{
public:
	CZone_Login(int ID, int ZoneID, int ProcID, int Maximum);
	~CZone_Login();
public:
	virtual void Process() override {};
private:
	CRITICAL_SECTION cs;
};