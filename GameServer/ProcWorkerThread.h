#pragma once
#include "MemoryManager/MemoryManager.h"
#include "CPlayer.h"

void CreateProcWorkerThread();
void WaitProcWorkerThread();
void PostMessageExit();


static unsigned __stdcall ProcLoginWorkerThread(void* arg);
static unsigned __stdcall ProcGameWorkerThread(void* arg);

class CProcWorker
{
public:
	CProcWorker() {};
	~CProcWorker() {};
private:
	int m_ProcID;

	CLockFreeQueue_MPSC<CPlayer*> m_PlayerDeleteQueue;


};