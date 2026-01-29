#pragma comment(lib, "winmm.lib")
#include "ProcWorkerThread.h"
#include "GameServerDef.h"
#include "NetWork/CNetServer.h"
#include "MemoryManager/MemoryManager.h"
#include "PacketProc.h"

#include <vector>
#include <Windows.h>
#include <process.h>

static std::vector<HANDLE> s_ProcWorkerThreadHandles;
static HANDLE s_hExit;
static PacketProc s_PacketProc;

void CreateProcWorkerThread()
{
	for (int i = 0; i < ProcLoginThreadCnt; i++)
    {
        HANDLE h = (HANDLE)_beginthreadex(NULL, 0, ProcLoginWorkerThread, 0, 0, NULL);
        s_ProcWorkerThreadHandles.push_back(h);
    }

    for (int i = 0; i < ProcMainThreadCnt; i++)
    {
        HANDLE h = (HANDLE)_beginthreadex(NULL, 0, ProcGameWorkerThread, 0, 0, NULL);
        s_ProcWorkerThreadHandles.push_back(h);
    }
    s_hExit = CreateEvent(NULL, true, NULL, NULL);
}

void WaitProcWorkerThread()
{
    WaitForMultipleObjects(s_ProcWorkerThreadHandles.size(), &s_ProcWorkerThreadHandles[0], true, INFINITE);

}

void PostMessageExit()
{
    
}

unsigned __stdcall ProcLoginWorkerThread(void* arg)
{
    timeBeginPeriod(1);
    int ret = 0;
    while (CNetServer::g_ServerON)
    {
        //1000 Frames 1초당 1000 처리
        ret = WaitForSingleObject(s_hExit, 1);

        // 종료 이벤트
        if (ret == WAIT_OBJECT_0)
            break;

        PROC_MSG job;
        while (g_ProcLoginJobQueue.TryDequeue(job))
        {
            CPlayer* pPlayer = g_PlayerManager[job.PlayerHandle];
            if (pPlayer == nullptr)
                continue;
            s_PacketProc.DO_GAME_Proc(job.type, pPlayer, job.packet);
            // 어떻게 해야 Player가 없어지지?
        }
    }
    return 0;
}

unsigned __stdcall ProcGameWorkerThread(void* arg)
{
    timeBeginPeriod(1);
    int ret = 0;
    while (CNetServer::g_ServerON)
    {
        //1000 Frames 1초당 1000 처리
        ret = WaitForSingleObject(s_hExit, 1);

        // 종료 이벤트
        if (ret == WAIT_OBJECT_0)
            break;

        PROC_MSG job;
        while (g_ProcJobQueue.TryDequeue(job))
        {
            CPlayer* pPlayer = g_PlayerManager[job.PlayerHandle];
            if (pPlayer == nullptr)
                continue;
            s_PacketProc.DO_GAME_Proc(job.type, pPlayer, job.packet);
			// 어떻게 해야 Player가 없어지지?
        }
    }
    return 0;
}
