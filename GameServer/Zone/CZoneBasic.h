#pragma once

#include "CZoneBase.h"
#include "../CPlayer.h"
#include "../MemoryManager/CLockFreeQueue_FromGPT.h"
#include "../CUtill/CEntityManagmentVector.h"
#include "../GameServerDef.h"

class CZoneBasic : public CZoneBase
{
public:
	CZoneBasic(int channel, int ZoneID, int ProcID, int Maximum);
	~CZoneBasic();

protected:
	CEntityVector m_vecEntitys;
	st_Vector3F m_stSpawnPos;

	virtual void OnEnterZone(CPlayer* pPlayer) {};
	virtual void OnLeaveZone(CPlayer* pPlayer) {};
private:
	CLockFreeQueue_MPSC<ZONE_CHANGE_JOB> m_queue;
	std::vector<ZONE_CHANGE_JOB> m_vecChangeZoneJobDebug;
public:
	virtual bool Teleport(CPlayer* pPlayer, st_Vector3F pos) { return true; };

public:
	void Init(int type, int width, int height);
	void ChangeZoneProcess();
	virtual void Process();

	virtual bool EnterZone(CPlayer* pPlayer);
	virtual bool LeaveZone(CPlayer* pPlayer);
	virtual bool PushMoveVector(CEntity* pEntity) { return false; };
	virtual void PopMoveVector(CEntity* pEntity) {};
	virtual st_Vector3F GetSpawnPos() { return m_stSpawnPos; };

	bool Enqueue(ZONE_CHANGE_JOB& job);
	bool TryPush(CPlayer* pPlayer);
	bool TryEnterZone();

	virtual void BoradCast(CPacket* pPacket, COORDINATE pivot, CPlayer* pPlayer = nullptr);
	virtual bool SendZoneInfo(CPlayer* pPlayer) { return true; };
};
