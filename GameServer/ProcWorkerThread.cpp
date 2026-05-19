#pragma comment(lib, "winmm.lib")
#include "ProcWorkerThread.h"
#include "GameServerDef.h"
#include "NetWork/CNetServer.h"
#include "Log/CLog.h"
#include "./ZoneManager/CZoneManager.h"
#include <Windows.h>
#include <process.h>
#include "CUtill/CUtill.h"
#include "Stub/EnumDef.h"

static std::vector<HANDLE> s_ProcWorkerThreadHandles;
static std::vector<int> s_ProcWorkerThreadIDs;

std::vector<CProcWorker*> s_ProcWorker;

CProcWorker g_MainProcWorker;

static HANDLE s_hExit;
static PacketProc s_PacketProc;

void CreateProcWorkerThread()
{
    s_ProcWorkerThreadHandles.clear();
    s_ProcWorkerThreadHandles.reserve(ProcThreadCnt);

    s_ProcWorkerThreadIDs.resize(ProcThreadCnt);

    int createmainworld = 0;

    for (int i = 0; i < ProcThreadCnt; i++)
    {
        s_ProcWorkerThreadIDs[i] = i;

		CProcWorker* pWorker = new CProcWorker();
		pWorker->Init(i, &s_PacketProc, &g_ProcJobQueue[i]);

		s_ProcWorker.push_back(pWorker);
        HANDLE h;
        if (i > 0 && createmainworld++ < ProcMainThreadCnt)
        {
            h = (HANDLE)_beginthreadex(NULL, 0, ProcMainWorldWorkerThread, &s_ProcWorkerThreadIDs[i], 0, NULL);
        }
        else
        {
            h = (HANDLE)_beginthreadex(NULL, 0, ProcWorkerThread, &s_ProcWorkerThreadIDs[i], 0, NULL);
        }
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

    s_ProcWorker[procID]->InitZoneVector();
    
    double accumulatedtime = 0.0f;
    double lasttime = CUtil::GetQPCNowTime();

    while (g_Net.GetRun())
    {
        //1000 Frames 1초당 1000 처리
        ret = WaitForSingleObject(s_hExit, 1);

        // 종료 이벤트
        if (ret == WAIT_OBJECT_0)
            break;

		// 패킷 처리
		s_ProcWorker[procID]->Proc();

        // 관리 Zone 이벤트
        s_ProcWorker[procID]->ZoneProc();

        // 지연 시간 누적
        double nowtime = CUtil::GetQPCNowTime();
        double frame = nowtime - lasttime;
        lasttime = nowtime;

        accumulatedtime += frame;

        int nLoop = 0;
        while (accumulatedtime >= FIXED_DELTA && nLoop < MAX_FRAME_LOOP_COUNT)
        {
            s_ProcWorker[procID]->ZoneUpdateByFrame();
            accumulatedtime -= FIXED_DELTA;
            nLoop++;
        }
        
        // 너무 많은 frame 이 쌓인다면 쌓인 frame 처리하느라 더 지연 최대 frame 만큼만 돌리기
        if (nLoop == MAX_FRAME_LOOP_COUNT)
        {
            g_LogServer.ELog("=== %d ProcThread AccumulateTime Warnning ===", procID);
            accumulatedtime = 0.0f;
        }
        
    }

    g_LogThread.ILog("=== END THREAD ProcWorkerThread ===");
    return 0;
}

unsigned __stdcall ProcMainWorldWorkerThread(void* arg)
{
    int procID = *(int*)arg;
    timeBeginPeriod(1);
    int ret = 0;

    s_ProcWorker[procID]->SetMain();
    s_ProcWorker[procID]->InitZoneVector();

    double accumulatedtime = 0.0f;
    double lasttime = CUtil::GetQPCNowTime();

    while (g_Net.GetRun())
    {
        //1000 Frames 1초당 1000 처리
        ret = WaitForSingleObject(s_hExit, 1);

        // 종료 이벤트
        if (ret == WAIT_OBJECT_0)
            break;

        // MainWorld 는 여기서 각각 실행GridThread 로 넣어 줘야한다
        s_ProcWorker[procID]->ProxyProc();
        s_ProcWorker[procID]->ZoneProc();
    }

    g_LogThread.ILog("=== END THREAD ProcWorkerThread ===");
    return 0;
}

void CProcWorker::Proc()
{
    PROC_MSG job;
	while (m_ProcJobQueue->TryDequeue(job))
    {
        CPlayer* pPlayer = g_Net.GetPlayer(job.PlayerHandle);
        if (pPlayer == nullptr)
            continue;
        if (pPlayer->GetZoneStatus() != eZONESTATUS::STABLE)
        {
            st_STC_ChangeingZone res;
            res.ret = ERROR_CODE::ZONE_CHANEING;
            res.type = job.type;

            pPlayer->SendPacket(res);
            continue;
        }
        m_pPacketProc->DO_GAME_Proc(job.type, pPlayer, job.packet);
    }
}

void CProcWorker::ProxyProc()
{
    PROC_MSG job;
    int max_dequeue = 1000;
    int dequeue = 0;
    std::vector<PROC_MSG> vec;
    while (m_ProcJobQueue->TryDequeue(job))
    {
        vec.push_back(job);

        // mainworld 인 경우 오는 패킷이 많을수 있음
        // 많은 dequeue 때문에 작업이 밀리수있어서 빨리 쳐준다
        if (dequeue++ >= max_dequeue)
            break;
    }

    // 기본 설정 잘못됨 MainWorld Thread 인데 다른게 들어옴
    if (!GetMain())
        exit(1);

    CMainWorld* pMainWorld = (CMainWorld*)GetMainWorld();
    pMainWorld->MessageRouting(vec);
}

void CProcWorker::ZoneProc()
{
    for (int i = 0; i < m_ZoneCnt; i++)
    {
        m_vecZone[i]->ZoneChangeJobProcess();
    }
}

void CProcWorker::ZoneUpdateByFrame()
{
    for (int i = 0; i < m_ZoneCnt; i++)
    {
        m_vecZone[i]->Process();
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
        //CNetServer::DecrementPlayerCount();
        
        g_ZoneManager.LeaveZone(pPlayer);
		g_Net.FreePlayer(pPlayer);   
    }
}

void CProcWorker::InitZoneVector()
{
    g_ZoneManager.InitProcZoneVector(m_ProcID, m_vecZone);
    m_ZoneCnt = m_vecZone.size();

    for (int i = 0; i < m_ZoneCnt; i++)
    {
        g_LogTemp.ILog("Proc[%d] Management ID[%d] Zone[%d] Proc[%d]"
            , m_ProcID
            , m_vecZone[i]->GetChannel(), m_vecZone[i]->GetZoneID(), m_vecZone[i]->GetProcID());
    }
}

CZoneBase* CProcWorker::GetMainWorld()
{
    if(!GetMain())
        return nullptr;

    return m_vecZone[0];
}

void DeleteProcWorker()
{
    for (auto pWorker : s_ProcWorker)
    {
        delete pWorker;
    }
    s_ProcWorker.clear();
}
