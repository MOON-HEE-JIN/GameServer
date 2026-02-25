#pragma once
#include "MemoryManager/MemoryManager.h"
#include "CPlayer.h"
#include "PacketProc.h"
#include <vector>
#include "ZoneManager/CZone.h"

void CreateProcWorkerThread();
void WaitProcWorkerThread();
void PostMessageProcThreadExit();
void DeleteProcWorker();

static unsigned __stdcall ProcWorkerThread(void* arg);

class CProcWorker
{
public:
	CProcWorker() {};
	~CProcWorker() {};

	void Init(int procID, PacketProc* pProc, CLockFreeQueue_MPSC<PROC_MSG>* pJobQ)
	{
		m_ProcID = procID;
		m_pPacketProc = pProc;
		m_ProcJobQueue = pJobQ;
	}
	
	void Proc();
	void ZoneProc();
	void ZonePlayerMoveUpdate();

	void ReleasePlayer(CPlayer* pPlayer)
	{
		m_PlayerDeleteQueue.Enqueue(pPlayer);
	}
	void DeletePlayerProcess();

	void InitZoneVector();
	void PushZone(CZone* pZone) { m_vecZone.push_back(pZone); }
private:
	int m_ProcID;
	CLockFreeQueue_MPSC<PROC_MSG>* m_ProcJobQueue;
	CLockFreeQueue_MPSC<CPlayer*> m_PlayerDeleteQueue;
	PacketProc* m_pPacketProc;
	std::vector<CZone*> m_vecZone;
	int m_ZoneCnt;
};

extern std::vector<CProcWorker*> s_ProcWorker;
