#include "CPlayer.h"
#include "NetWork/CNetServer.h"
#include "ZoneManager/CZoneManager.h"

void CPlayer::Init(SESSION_HANDLE sessionID, int handle, int procID)
{
	m_SessionHandle = sessionID;
	m_PlayerHandle = handle;
	m_OwnerZone = procID;

	m_bRelease.store(false);
}


void CPlayer::SetRelease()
{
	m_bRelease.store(true);
	SessionHandleClear();
}

void CPlayer::SendPacket(CPacket* pPacket)
{
	TrySend(m_SessionHandle, pPacket);
}

