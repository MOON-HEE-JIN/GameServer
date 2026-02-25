#pragma once

#include "CUtill/CPacket.h"
#include "NetWork/NetWorkDefine.h"
#include "GameServerDef.h"
#include "CEntity.h"

class CPlayer : public CEntity
{
public:
	CPlayer() {};
	~CPlayer() {};

	void Init(SESSION_HANDLE sessionID, int handle, int procID);

	void Clear() {};
private:
	SESSION_HANDLE m_SessionHandle;
	int m_PlayerHandle;									// Player 전체 에 대한 handle
	std::atomic<int> m_OwnerZone;						// 처리 Zone 에 대한 id
	eZONESTATUS m_eZoneStatus;							// 현재 Zone 에 서 의 상태
	std::atomic<bool> m_bRelease;						// 삭제 처리중

private:
	void SessionHandleClear() { m_SessionHandle = SESSION_HANDLE(-1, 0); }
	
public:
	SESSION_HANDLE GetSessionHandle() { return m_SessionHandle; }
	int GetPlayerHandle() { return m_PlayerHandle; }
	int GetZoneID() { return m_OwnerZone.load(); }
	bool GetRelease() { return m_bRelease.load(); }
	eZONESTATUS GetZoneStatus() { return m_eZoneStatus; }
	
	void SetZoneID(int zone) { m_OwnerZone.store(zone); };
	void SetZoneStatus(eZONESTATUS type) { m_eZoneStatus = type; }
	void SetRelease();
public:
	void EnterZone();		// Zone 에 입장시 주위 플레이어 들에게 알림 && 주위 플레이어 알림

public:
	void SendPacket(CPacket* pPacket);
	template<typename T>
	void SendPacket(T& value)
	{
		CPacket pack;
		pack << value;
		SendPacket(&pack);
	}
};
