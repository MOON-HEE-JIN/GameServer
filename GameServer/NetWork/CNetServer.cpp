#include "CNetServer.h"
#include <process.h>
#include <ws2tcpip.h>
#include <iostream>
#include<atomic>

#include "NetWorkDefine.h"
#include "../ProcWorkerThread.h"
#include "../Log/CLog.h"
#include "CGMSession.h"
#include "../ZoneManager/CZoneManager.h"

CNetServer g_Net;

bool CNetServer::OnClientJoin(CSession* pSession)
{
	if (pSession == nullptr)
		return false;

	CPlayer* pPlayer = nullptr;
	int PlayerHandle;

	pPlayer = AllocPlayer(PlayerHandle);
	if(pPlayer == nullptr)
		return false;

	m_iPlayerConnectCount.fetch_add(1);

	pPlayer->Init(pSession->GetConnectKey(), PlayerHandle, 0, 0);
	pSession->SetConnectPlayerHandle(PlayerHandle);

	//g_LogServer.ILog("OnClientJoin SessionHandle : %d, PlayerHandle : %d" , pSession->GetConnectHandle(), PlayerHandle);
	
	if (!g_ZoneManager.ReqEnterLoginZone(pPlayer))
	{
		pSession->SetConnectPlayerHandle(-1);
		FreePlayer(pPlayer);
		
		m_iPlayerConnectCount.fetch_sub(1);

		return false;
	}
	
	//pPlayer->SetZoneStatus(eZONESTATUS::STABLE);

	st_STC_ConnectInfo pack;
	pack.info.ID = PlayerHandle;

	pPlayer->SendPacket(pack);
	return true;
}

void CNetServer::OnDisconnect(CSession* pSession)
{
	CPlayer* pPlayer = g_PlayerManager[pSession->GetConnectPlayerHandle()];
	if (pPlayer != nullptr)
	{
		pPlayer->SetRelease();
		pPlayer->ReleaseRef();
		ZONE_CHANGE_JOB z(GetTickCount(), eZONESTATUS::RELEASE, pPlayer->GetID()
			, pPlayer->GetChannel(), pPlayer->GetZoneID()
			, pPlayer->GetChannel(), pPlayer->GetZoneID()
			, 0, 0);

		EnqueueChangeJob(pPlayer->GetChannel(), pPlayer->GetZoneID(), z);
	}
}

void CNetServer::OnRecv(CSession* pSession, int type, CPacket& packet)
{
	int pid = pSession->GetProcID();
	if (pid < 0 || pid >= ProcThreadCnt)
	{
		g_LogServer.ELog("Invalid ProcID on recv. Session:%d ProcID:%d", pSession->GetConnectHandle(), pid);

		pSession->CloseSocket();

		if (pSession->GetIOCnt() == 0 && pSession->GetRefCnt() == 0 && pSession->GetBoolbCloseing())
		{
			DisConnect(pSession);
		}

		return;
	}

	g_ProcJobQueue[pid].Enqueue({ pSession->GetConnectPlayerHandle(),type, packet });
}

int CNetServer::Initializer(int Port, int RunWorkerThreadCount)
{
	int ret = Init(Port, RunWorkerThreadCount);
	if (ret != 0)
		return ret;
	
	InitializeCriticalSection(&g_csPlayerManager);

	g_PlayerManager.resize(MAX_CONNECT_COUNT);
	for (int i = 0; i < MAX_CONNECT_COUNT; i++)
		g_PlayerManager[i] = nullptr;
	g_PlayerHandleManager.resize(MAX_CONNECT_COUNT);
	for (int i = 0; i < MAX_CONNECT_COUNT; i++)
	{
		g_PlayerHandleManager.push_back(MAX_CONNECT_COUNT - 1 - i);
	}

	m_iLogDelayTime = 1 * 1000;
	m_iLogTime = 0;

	return 0;
}

int CNetServer::Start()
{
	StartServer(this);

	CreateProcWorkerThread();
	CreateLogThread();

	return 0;
}

int CNetServer::Wait()
{
	WaitStopServer();
	WaitProcWorkerThread();
	WaitLogThread();

	return 0;
}

int CNetServer::ShutDown()
{
	ServerShutDown();
	PostMessageProcThreadExit();
	PostMessageLogThreadExit();

	return 0;
}

bool TryChangeZone(const SESSION_HANDLE& key, int ProcID)
{
	CSession* pSession = g_Net.GetSession(key);
	if (pSession == nullptr)
		return false;

	// 연결 및 재사용 횟수 체크
	if (!pSession->GetBoolConnect()) return false;
	if (pSession->GetConnectGen() != key.Gen) return false;

	if (!pSession->AddRef()) return false;

	if (!pSession->GetBoolConnect() || pSession->GetConnectGen() != key.Gen || pSession->GetBoolbCloseing())
	{
		pSession->SubRef();
		return false;
	}

	bool bRet = pSession->SetProcID(ProcID);
	pSession->SubRef();
	return bRet;
}

bool TrySend(const SESSION_HANDLE& key, CPacket* pPacket)
{
	CSession* pSession = g_Net.GetSession(key);
	if (pSession == nullptr)
		return false;

	SESSION_HANDLE CurKey = pSession->GetConnectKey();
	
	// 연결 및 재사용 횟수 체크
	if (!pSession->GetBoolConnect()) return false;
	if (pSession->GetConnectGen() != key.Gen) return false;

	if (!pSession->AddRef()) return false;
	
	if (!pSession->GetBoolConnect() || pSession->GetConnectGen() != key.Gen || pSession->GetBoolbCloseing())
	{
		pSession->SubRef();
		return false;
	}

	pSession->SendPacket(pPacket);
	pSession->SubRef();

	return true;
}


CPlayer* CNetServer::GetPlayer(int handle)
{
	if (handle < 0 || handle >= MAX_CONNECT_COUNT)
		return nullptr;

	return g_PlayerManager[handle];
}

CPlayer* CNetServer::AllocPlayer(int& outPlayerHandle)
{
	int key = -1;
	EnterCriticalSection(&g_csPlayerManager);
	{
		if (!g_PlayerHandleManager.empty())
		{
			key = g_PlayerHandleManager.back();
			g_PlayerHandleManager.pop_back();
		}
	}
	LeaveCriticalSection(&g_csPlayerManager);

	if (key < 0)
		return nullptr;

	if (g_PlayerManager[key] == nullptr)
	{
		CPlayer* pPlayer = new CPlayer;
		g_PlayerManager[key] = pPlayer;
		outPlayerHandle = key;
		return pPlayer;
	}
	else
	{
		outPlayerHandle = key;
		return g_PlayerManager[key];
	}

	return nullptr;
}

void CNetServer::FreePlayer(CPlayer* pPlayer)
{
	pPlayer->ReleaseRef();
}

void CNetServer::AddPlayerHandle(int handle)
{
	EnterCriticalSection(&g_csPlayerManager);
	{
		g_PlayerHandleManager.push_back(handle);
	}
	LeaveCriticalSection(&g_csPlayerManager);
	m_iPlayerConnectCount.fetch_sub(1);
}

void CNetServer::NetLog()
{
	if (m_iLogTime + m_iLogDelayTime < GetTickCount())
	{
		g_LogServer.ILog("================\n \
			IO Count  Recv : %d[%d], Send : %d[%d]\n \
			SessionCount : %d, PlayerCount : %d\n \
			================"
			, GetRecvOverlappedCount(), GetRecvOverlappedSize(), GetSendOverlappedCount(), GetSendOverlappedSize()
			, GetConnectionSessionCount(), GetPlayerCount());
		ResetRecvOverlappedLog();
		ResetSendOverlappedLog();

		m_iLogTime = GetTickCount();
	}
}

void CNetServer::PlayerDisConnect(const SESSION_HANDLE& key)
{
	CSession* pSession = g_Net.GetSession(key);
	if (pSession == nullptr)
		return ;

	SESSION_HANDLE CurKey = pSession->GetConnectKey();

	// 연결 및 재사용 횟수 체크
	if (!pSession->GetBoolConnect()) return ;
	if (pSession->GetConnectGen() != key.Gen) return ;

	if (!pSession->AddRef()) return;
	
	DisConnect(pSession);

	pSession->SubRef();

	return ;
}