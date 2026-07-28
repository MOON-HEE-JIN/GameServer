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
	CPlayer()
		: m_iVarRef(0), m_SessionHandle(SESSION_HANDLE()), m_PlayerHandle(-1),
		m_bRelease(true), m_pZone(nullptr) {};
	~CPlayer() {};

	void Init(SESSION_HANDLE sessionID, int handle, int Channel, int Zone);

private:
	std::atomic<int> m_iVarRef;
	std::atomic<SESSION_HANDLE> m_SessionHandle;
	int m_PlayerHandle;									// Player 전체 에 대한 handle
	
	std::atomic<bool> m_bRelease;						// 삭제 처리중

	CZoneBase* m_pZone;
private:
	void SessionHandleClear() { m_SessionHandle.store(SESSION_HANDLE(-1, 0)); }
	void Clear();

public:
	void AddVarRef();
	void ReleaseVarRef();
protected:
	virtual void OnRelease() override;
public:
	SESSION_HANDLE GetSessionHandle() { return m_SessionHandle.load(); }
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

public:
	//Teleport
	bool Teleport(st_Vector3F pos);
};
