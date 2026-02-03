#include "CPlayer.h"
#include "NetWork/CNetServer.h"


void CPlayer::Init(SESSION_HANDLE sessionID, int handle, int procID)
{
	m_SessionHandle = sessionID;
	m_PlayerHandle = handle;
	m_ProcID = procID;
}

void CPlayer::SendPacket(CPacket* pPacket)
{
}
