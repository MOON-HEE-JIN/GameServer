#pragma once

#include "CUtill/CPacket.h"
#include "NetWork/NetWorkDefine.h"

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
  int m_ZoneID;						    // 처리 Zone 에 대한 id

public:
	SESSION_HANDLE GetSessionHandle() { return m_SessionHandle; }
	int GetPlayerHandle() { return m_PlayerHandle; }
	int GetZoneID() { return m_ZoneID; }

	void SessionHandleClear() { m_SessionHandle = SESSION_HANDLE(-1, 0); }
	void SetZoneID(int zone) { m_ZoneID = zone; };
public:
	void SendPacket(CPacket* pPacket);
};
