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
	int m_PlayerHandle;
	int m_ProcID;

public:
	SESSION_HANDLE GetSessionHandle() { return m_SessionHandle; }
	int GetPlayerHandle() { return m_PlayerHandle; }
	int GetProcID() { return m_ProcID; }
	
	void SessionHandleClear() { m_SessionHandle = SESSION_HANDLE(-1, 0); }
	void ChangeProcID(int pid);
	
public:
	void SendPacket(CPacket* pPacket);
};