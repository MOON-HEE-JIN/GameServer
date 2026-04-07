#pragma once
#include "CZone.h"

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
// Packet 처리를 위해서 Lock 을 걸고 넣어 준다
class CZone_Login : public CZone
{
public:
	CZone_Login(int managerIndex, int pid, int max, const char* name);
	~CZone_Login();
private:
	CRITICAL_SECTION cs;

public:
	virtual bool EnterZone(CPlayer* pPlayer);
	virtual bool LeaveZone(CPlayer* pPlayer);
};