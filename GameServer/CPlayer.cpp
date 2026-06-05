#include "CPlayer.h"
#include "NetWork/CNetServer.h"
#include "ZoneManager/CZoneManager.h"
#include "Log/CLog.h"

void CPlayer::Init(SESSION_HANDLE sessionID, int handle, int Channel, int Zone)
{
	m_iRef.store(1);

	m_SessionHandle = sessionID;
	m_PlayerHandle = handle;
	m_iChannel = Channel;
	m_OwnerZone = Zone;

	m_bRelease.store(false);

	m_iGridID = -1;
}

void CPlayer::Clear()
{
	Reset();

	m_SessionHandle = { -1, 0 };
	m_PlayerHandle = -1;
	m_iChannel = 0;
	m_OwnerZone = 0;

	m_iGridID = -1;

	m_bRelease.store(false);
}

void CPlayer::AddRef(int reason)
{
	m_iRef.fetch_add(1);
	m_iRefReason.push_back(reason);
	if (m_iRef.load() == 6)
	{
		int a = 100;
		a++;
	}
}

void CPlayer::ReleaseRef(int reason)
{
	m_iRef.fetch_sub(1);
	if (m_iRefReasonCount.size() > 0)
	{
		if (m_iRefReasonCount[0] == reason)
		{
			int a = 100;
			a++;
		}
	}
	m_iRefReasonCount.push_back(reason);
	if (m_iRef.load() > 0)
		return;

	int key = GetID();
	Clear();
	g_Net.AddPlayerHandle(key);
	
	g_LogGame.DLog("Player Handle %d ReleaseRef", key);
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

bool CPlayer::Teleport(st_Vector3F pos)
{
	st_Vector3F originPos = m_stPosition;

	if (!((CZoneBasic*)m_pZone)->Teleport(this, pos))
	{
		m_stPosition = originPos;
		return false;
	}
	return true;
}

