#pragma comment(lib, "winmm.lib")
#include "ProcWorkerThread.h"
#include "GameServerDef.h"
#include "NetWork/CNetServer.h"
#include "Log/CLog.h"
#include "./ZoneManager/CZoneManager.h"
#include <Windows.h>
#include <process.h>

static std::vector<HANDLE> s_ProcWorkerThreadHandles;
static std::vector<int> s_ProcWorkerThreadIDs;

std::vector<CProcWorker*> s_ProcWorker;

static HANDLE s_hExit;
static PacketProc s_PacketProc;

void CreateProcWorkerThread()
{
    s_ProcWorkerThreadHandles.clear();
    s_ProcWorkerThreadHandles.reserve(ProcThreadCnt);

    s_ProcWorkerThreadIDs.resize(ProcThreadCnt);

    for (int i = 0; i < ProcThreadCnt; i++)
    {
        s_ProcWorkerThreadIDs[i] = i;

		CProcWorker* pWorker = new CProcWorker();
		pWorker->Init(i, &s_PacketProc, &g_ProcJobQueue[i]);

		s_ProcWorker.push_back(pWorker);
        HANDLE h = (HANDLE)_beginthreadex(NULL, 0, ProcWorkerThread, &s_ProcWorkerThreadIDs[i], 0, NULL);
        s_ProcWorkerThreadHandles.push_back(h);
    }

    // manual-reset(TRUE), 초기 비신호(FALSE)
    s_hExit = CreateEvent(NULL, TRUE, FALSE, NULL);
}

void WaitProcWorkerThread()
{
    if (!s_ProcWorkerThreadHandles.empty())
    {
        WaitForMultipleObjects((DWORD)s_ProcWorkerThreadHandles.size(), &s_ProcWorkerThreadHandles[0], TRUE, INFINITE);
        for (HANDLE h : s_ProcWorkerThreadHandles)
        {
            if (h) CloseHandle(h);
        }
        s_ProcWorkerThreadHandles.clear();
    }

    DeleteProcWorker();

    if (s_hExit)
    {
        CloseHandle(s_hExit);
        s_hExit = NULL;
    }
}

void PostMessageProcThreadExit()
{
	// 종료 이벤트 발생
	SetEvent(s_hExit);
}


unsigned __stdcall ProcWorkerThread(void* arg)
{
	int procID = *(int*)arg;
    timeBeginPeriod(1);
    int ret = 0;
    while (CNetServer::g_ServerON)
    {
        //1000 Frames 1초당 1000 처리
        ret = WaitForSingleObject(s_hExit, 1);

        // 종료 이벤트
        if (ret == WAIT_OBJECT_0)
            break;

		// 패킷 처리
		s_ProcWorker[procID]->Proc();



		// 플레이어 삭제 처리
		s_ProcWorker[procID]->DeletePlayerProcess();
    }

    g_LogThread.ILog("=== END THREAD ProcWorkerThread ===");
    return 0;
}

void CProcWorker::Proc()
{
    PROC_MSG job;
	while (m_ProcJobQueue->TryDequeue(job))
    {
        CPlayer* pPlayer = g_PlayerManager[job.PlayerHandle];
        if (pPlayer == nullptr)
            continue;
        m_pPacketProc->DO_GAME_Proc(job.type, pPlayer, job.packet);
    }
}

void CProcWorker::DeletePlayerProcess()
{
    CPlayer* pPlayer = nullptr;
    while (m_PlayerDeleteQueue.TryDequeue(pPlayer))
    {
		if (pPlayer == nullptr)
            continue;
        //g_LogGame.ILog("Release Player PHandle : %d, SHandle : %d", pPlayer->GetPlayerHandle(), pPlayer->GetSessionHandle().Handle);
        CNetServer::DecrementPlayerCount();
        g_ZoneManager.LeaveZone(pPlayer);
        pPlayer->SessionHandleClear();
		FreePlayer(pPlayer);   
    }
}

void DeleteProcWorker()
{
    for (auto pWorker : s_ProcWorker)
    {
        delete pWorker;
    }
    s_ProcWorker.clear();
}