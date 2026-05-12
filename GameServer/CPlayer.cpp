#include "CPlayer.h"
#include "NetWork/CNetServer.h"
#include "ZoneManager/CZoneManager.h"

void CPlayer::Init(SESSION_HANDLE sessionID, int handle, int Channel, int Zone)
{
	m_SessionHandle = sessionID;
	m_PlayerHandle = handle;
	m_iChannel = Channel;
	m_OwnerZone = Zone;

	m_bRelease.store(false);
}

void CPlayer::Clear()
{
	Reset();

	m_SessionHandle = { -1, 0 };
	m_PlayerHandle = -1;
	m_iChannel = 0;
	m_OwnerZone = 0;

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

