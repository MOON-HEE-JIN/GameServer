#pragma once

#include "CUtill/CPacket.h"
#include "NetWork/NetWorkDefine.h"
#include "CEntity.h"
#include "GameServerDef.h"
#include "Zone/CZoneBase.h"

class CZoneBase;

class CPlayer : public CEntity
{
public:
	CPlayer() {};
	~CPlayer() {};

	void Init(SESSION_HANDLE sessionID, int handle, int Channel, int Zone);

private:
	/*
	* m_iRef 사용처
	* - OnClientJoin 에서 1 로 시작 FreePlayer 에서 -1
	* - ReqEnterLoginZone +1 이유 순서 Leave -> Enter +1 을 하지 않으면 바로 종료
	* - EnterZone 에서 +1, LeaveZone 에서 -1
	* - Grid AddPlayer +1, RemovePlayer -1
	*/
	std::atomic<int> m_iRef;
	SESSION_HANDLE m_SessionHandle;
	int m_PlayerHandle;									// Player 전체 에 대한 handle
	
	std::atomic<bool> m_bRelease;						// 삭제 처리중

	CZoneBase* m_pZone;
private:
	void SessionHandleClear() { m_SessionHandle = SESSION_HANDLE(-1, 0); }
	void Clear();

public:
	int GetRef() { return m_iRef.load(); }
	void AddRef();
	void ReleaseRef();

	SESSION_HANDLE GetSessionHandle() { return m_SessionHandle; }
	virtual int GetID() { return m_PlayerHandle; }
	bool GetRelease() { return m_bRelease.load(); }

	void SetRelease();
	void SetZone(CZoneBase* pZone) { m_pZone = pZone; }
public:
	void SendPacket(CPacket* pPacket);
	template<typename T>
	void SendPacket(T& value)
	{
		CPacket pack;
		pack << value;
		SendPacket(&pack);
	}
	void BroadCast(CPacket* pPacket);
	template<typename T>
	void BroadCast(T& value)
	{
		CPacket pack;
		pack << value;
		BroadCast(&pack);
	}
public:
	//Teleport
	bool Teleport(st_Vector3F pos);
};
