#include "CPlayer.h"
#include "NetWork/CNetServer.h"


void CPlayer::Init(SESSION_HANDLE sessionID, int handle, int procID)
{
	m_SessionHandle = sessionID;
	m_PlayerHandle = handle;
	m_ProcID = procID;
}

void CPlayer::ChangeProcID(int pid)
{
	CNetServer::DecrementProcCount(m_ProcID);
	m_ProcID = pid;
	CNetServer::IncrementProcCount(m_ProcID);
}

void CPlayer::SendPacket(CPacket* pPacket)
{
}
