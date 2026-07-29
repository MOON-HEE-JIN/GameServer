#include "CPlayer.h"
#include "NetWork/CNetServer.h"
#include "ZoneManager/CZoneManager.h"
#include "Log/CLog.h"

void CPlayer::Init(SESSION_HANDLE sessionID, int handle, int Channel, int Zone)
{
	m_iVarRef.store(0);
	
	m_nEntityType = eENTITY_TYPE::ENTITY_PLAYER;
	m_SessionHandle.store(sessionID);
	m_PlayerHandle = handle;
	m_iChannel = Channel;
	m_OwnerZone = Zone;

	m_bRelease.store(false);

	m_iGridID = -1;

	AddVarRef();
}

void CPlayer::Clear()
{
	Reset();

	m_SessionHandle.store(SESSION_HANDLE(-1, 0));
	m_PlayerHandle = -1;
	m_iChannel = 0;
	m_OwnerZone = 0;

	m_iGridID = -1;

	m_bRelease.store(false);
}

void CPlayer::AddVarRef()
{
	m_iVarRef.fetch_add(1);
	AddRef();
}

void CPlayer::ReleaseVarRef()
{
	int ref = m_iVarRef.fetch_sub(1);
	if (ref <= 0)
	{
		m_iVarRef.fetch_add(1);
		g_LogRef.ELog("Player Handle %d ReleaseVarRef Error", GetID());
		return;
	}

	ReleaseRef();
}

void CPlayer::OnRelease()
{
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
	TrySend(m_SessionHandle.load(), pPacket);
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

