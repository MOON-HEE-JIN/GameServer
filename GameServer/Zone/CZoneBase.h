#pragma once

#include "../CPlayer.h"
#include "../MemoryManager/CLockFreeQueue_FromGPT.h"
#include "../GameServerDef.h"

#include <vector>
#include <unordered_map>

class CPlayer;

class CZoneBase
{
public:
	CZoneBase(int channel, int ZoneID, int ProcID, int Maximum);
	~CZoneBase();
	
private:
	int m_iChannel;
	int m_iZoneID;
	int m_iProcID;
	int m_iMaximumUser;

	std::atomic<int> m_iCount;
protected:
	bool m_bMainWorld = false;
	std::atomic<bool> m_bActive;
	std::vector<CPlayer*> m_vecPlayers;
	
	int m_iWidth;
	int m_iHeight;
private:
	std::unordered_map<int, int> m_mapIDtoIndex;

	CLockFreeQueue_MPSC<ZONE_CHANGE_JOB> m_queue;

public:
	virtual void Init(int ID, int ZoneID, int ProcID, int Maximum);
	virtual void Reset();

	void ZoneChangeJobProcess();
	virtual void Process() = 0;
public:
	int GetChannel() { return m_iChannel; }
	int GetZoneID() { return m_iZoneID; }
	int GetProcID() { return m_iProcID; }
	int GetMaximum() { return m_iMaximumUser; }
	int GetCurCnt() { return m_iCount.load(); }

	int GetWidth() { return m_iWidth; }
	int GetHeight() { return m_iHeight; }
	bool GetMainWorld() { return m_bMainWorld; }
public:
	bool CheckPos(st_Vector3F pos);
public:
	bool Enqueue(ZONE_CHANGE_JOB& job);
	bool TryPush(CPlayer* pPlayer);
	virtual bool EnterZone(CPlayer* pPlayer);
	virtual bool LeaveZone(CPlayer* pPlayer);
	virtual void PushMoveVector(CEntity* pEntity) {};
	bool TryEnterZone();
protected:
	virtual void OnEnterZone(CPlayer* pPlayer) {};
	virtual void OnLeaveZone(CPlayer* pPlayer) {};
public:
	virtual bool Teleport(CPlayer* pPlayer, st_Vector3F pos) { return true; };

	void SendBoradCast(CPacket* pPacket, CPlayer* pPlayer = nullptr);
	virtual bool SendZoneInfo(CPlayer* pPlayer) { return true; };
};