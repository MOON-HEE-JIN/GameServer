#pragma once

#include "CUtill/CPacket.h"
#include "NetWork/NetWorkDefine.h"
#include "GameServerDef.h"

class CPlayer
{
public:
	CPlayer() {};
	~CPlayer() {};

	void Init(SESSION_HANDLE sessionID, int handle, int procID);

	void Clear() {};
private:
	SESSION_HANDLE m_SessionHandle;
	int m_PlayerHandle;					// Player 전체 에 대한 handle
	int m_ZoneID;						// 처리 Zone 에 대한 id
	eZONESTATUS m_eZoneStatus;			// 현재 Zone 에 서 의 상태
	std::atomic<int> m_RefCnt;
	std::atomic<bool> m_bRelease;

private:
	void SessionHandleClear() { m_SessionHandle = SESSION_HANDLE(-1, 0); }

public:
	SESSION_HANDLE GetSessionHandle() { return m_SessionHandle; }
	int GetPlayerHandle() { return m_PlayerHandle; }
	int GetZoneID() { return m_ZoneID; }
	eZONESTATUS GetZoneStatus() { return m_eZoneStatus; }
	
	void SetZoneID(int zone) { m_ZoneID = zone; };
	void SetZoneStatus(eZONESTATUS type) { m_eZoneStatus = type; }
	void SetRelease();
public:
	void SendPacket(int type, CPacket* pPacket);
};
