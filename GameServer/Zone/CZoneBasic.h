#pragma once

#include "CZoneBase.h"
#include "../CPlayer.h"
#include "../MemoryManager/CLockFreeQueue_FromGPT.h"

#include <vector>
#include <unordered_map>

class CZoneBasic : public CZoneBase
{
public:
	CZoneBasic(int channel, int ZoneID, int ProcID, int Maximum);
	~CZoneBasic();

protected:
	std::vector<CPlayer*> m_vecPlayers;

	virtual void OnEnterZone(CPlayer* pPlayer) {};
	virtual void OnLeaveZone(CPlayer* pPlayer) {};
private:
	CLockFreeQueue_MPSC<ZONE_CHANGE_JOB> m_queue;

public:
	virtual bool Teleport(CPlayer* pPlayer, st_Vector3F pos) { return true; };

public:
	void Init(int type, int width, int height);
	void ChangeZoneProcess();
	virtual void Process();

	virtual bool EnterZone(CPlayer* pPlayer);
	virtual bool LeaveZone(CPlayer* pPlayer);
	virtual void PushMoveVector(CEntity* pEntity) {};

	bool Enqueue(ZONE_CHANGE_JOB& job);
	bool TryPush(CPlayer* pPlayer);
	bool TryEnterZone();

	void SendZoneCast(CPacket* pPacket, CPlayer* pPlayer = nullptr);
	virtual bool SendZoneInfo(CPlayer* pPlayer) { return true; };
};