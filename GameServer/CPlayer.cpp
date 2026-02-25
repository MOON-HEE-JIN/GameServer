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

void CPlayer::EnterZone()
{
	CPacket pack;
	st_STC_CreateChar create;
	create.ID = m_PlayerHandle;
	create.pos.X = 0;
	create.pos.Y = 0;

	pack << create;
	// 본인 포함 Player 생성 메시지
	g_ZoneManager.SendZone(m_OwnerZone, &pack);
}

void CPlayer::SendPacket(CPacket* pPacket)
{
	TrySend(m_SessionHandle, pPacket);
}

