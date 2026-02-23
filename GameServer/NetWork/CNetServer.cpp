癤#include "CNetServer.h"
#include <process.h>
#include <ws2tcpip.h>
#include <iostream>
#include<atomic>

#include "NetWorkDefine.h"
#include "../ProcWorkerThread.h"
#include "../Log/CLog.h"
#include "CGMSession.h"
#include "../ZoneManager/CZoneManager.h"

unsigned short CNetServer::Port = 7799;
unsigned short CNetServer::GmPort = 7700;
SOCKET CNetServer::listen_sock;
SOCKET CNetServer::gm_listen_sock;

int CNetServer::AcceptCnt;
std::vector<CSession*> CNetServer::SessionManager;
std::vector<SESSION_HANDLE> CNetServer::SessionFreeKey;
__int64 CNetServer::AcceptKey;
CRITICAL_SECTION CNetServer::cs_SessionFreeKey;

unsigned __int64 CNetServer::AllocSessionID;
bool CNetServer::g_ServerON;
HANDLE CNetServer::CICP;
HANDLE CNetServer::h_AceeptThread;
HANDLE CNetServer::h_GmAceeptThread;
HANDLE* CNetServer::h_WorkerThread;

std::vector<CPlayer*> g_PlayerManager;
std::vector<int> g_PlayerHandleManager;
CRITICAL_SECTION g_csPlayerManager;

static std::atomic<bool> s_bSessionSendEnqueueRunning = false;
static std::atomic<bool> s_bSessionDisConnectDequeueRunning = false;

std::atomic<int> CNetServer::ConnectSessionCount;
std::atomic<int> CNetServer::TotalConnectSessionCount;
std::atomic<int> CNetServer::ConnectPlayerCount;
std::atomic<int> CNetServer::TotalConnectPlayerCount;
std::vector<std::atomic<int>> CNetServer::ConnectProcCount(ProcThreadCnt);

int CNetServer::LogPrintTime;
int CNetServer::LogPrintDelay = 3 * 1000;

void OnRecv(CSession* pSession, int type, CPacket pPacket)
{
	if (pSession == nullptr)
		return;

	int pid = pSession->GetProcID();
	if (pid < 0 || pid >= ProcThreadCnt)
	{
		g_LogServer.ELog("Invalid ProcID on recv. Session:%d ProcID:%d", pSession->GetConnectHandle(), pid);
		EnqueueDisConnectReq(pSession);
		return;
	}

	//g_LogServer.DLog("Enqueue Job type : %d, size : %d", type, pPacket.GetDataSize());
	g_ProcJobQueue[pid].Enqueue({pSession->GetConnectPlayerHandle(),type, pPacket});
}

bool OnClientJoin(CSession* pSession)
{
	if (pSession == nullptr)
		return false;

	CPlayer* pPlayer = nullptr;
	int PlayerHandle;

	pPlayer = AllocPlayer(PlayerHandle);
	if(pPlayer == nullptr)
		return false;

	CNetServer::IncrementPlayerCount();
	pPlayer->Init(pSession->GetConnectKey(), PlayerHandle, pSession->GetZoneID());
	pSession->SetConnectPlayerHandle(PlayerHandle);

	//g_LogServer.ILog("OnClientJoin SessionHandle : %d, PlayerHandle : %d" , pSession->GetConnectHandle(), PlayerHandle);

	if (!g_ZoneManager.EnterZone(pPlayer, 0))
	{
		pSession->SetConnectPlayerHandle(-1);
		FreePlayer(pPlayer);
		CNetServer::DecrementPlayerCount();
		return false;
	}
	pPlayer->SetZoneStatus(eZONESTATUS::STABLE);
	return true;
}

unsigned __stdcall AceeptThread(void* arg)
{
	int retval;
	int addrlen;
	SOCKADDR_IN clientaddr;
	SOCKET client_sock;
	HANDLE IOretval;

	while (CNetServer::g_ServerON)
	{
		addrlen = sizeof(clientaddr);
		client_sock = accept(CNetServer::GetListenSocket(), (SOCKADDR*)&clientaddr, &addrlen);
		
		if (client_sock == INVALID_SOCKET)
		{
			retval = GetLastError();
			g_LogServer.ELog("Accept Error %d", retval);
			continue;
		}
		CNetServer::IncrementAcceptCnt();

		CSession* pSession = CNetServer::AddSession(client_sock);
		if (pSession == nullptr)
		{
			closesocket(client_sock);
			continue;
		}

		IOretval = CreateIoCompletionPort((HANDLE)client_sock, CNetServer::GetCICP(), (ULONG_PTR)pSession, 0);

		int IOError = WSAGetLastError();
		if (IOretval == NULL)
		{
			CNetServer::DisConnect(pSession);
			g_LogServer.ELog("Accept CICP Error %d", IOError);
			continue;
		}

		if (!OnClientJoin(pSession))
		{
			CNetServer::DisConnect(pSession);
			continue;
		}

		CNetServer::IncrementSessionCount();

		if(!pSession->RecvPost())
			CNetServer::DisConnect(pSession);
		
	}
	g_LogThread.ILog("=== END THREAD AcceptThread ===");
	return 0;
}

unsigned __stdcall GMAceeptThread(void* arg)
{
	int retval;
	int addrlen;
	SOCKADDR_IN clientaddr;
	SOCKET client_sock;

	while (CNetServer::g_ServerON)
	{
		addrlen = sizeof(clientaddr);
		client_sock = accept(CNetServer::GetGmListenSocket(), (SOCKADDR*)&clientaddr, &addrlen);

		if (client_sock == INVALID_SOCKET)
		{
			retval = GetLastError();
			if (CNetServer::g_ServerON)
			{
				g_LogServer.ELog("GM Accept Error %d", retval);
			}
			continue;
		}
		g_LogServer.ILog("GM Connect");
		// 관리 소케은 동기 로 관리
		CGMSession gmSession(client_sock);
		gmSession.Run();
	}

	return 0;
}

unsigned __stdcall WorkerThread(void* arg)
{
	int retval = 0;
	CSession* pSession = nullptr;
	DWORD transfrerred;

	ULONG_PTR key;
	while (CNetServer::g_ServerON)
	{
		pSession = nullptr;
		transfrerred = 0;
		OVERLAPPED* overlapped = nullptr;
		retval = GetQueuedCompletionStatus(CNetServer::GetCICP(), &transfrerred, &key, &overlapped, INFINITE);
		/*
		if (pSession == nullptr && transfrerred == NULL && overlapped == nullptr)
		{
			break;
		}
		*/
		
		if (key == KEY_SEND_WAKE)
		{
			SessionSendQEnqueue();
			continue;
		}

		if (key == KEY_DISCONNECT_WAKE)
		{
			DequeueDisConnectReq();
			continue;
		}

		if (key == KEY_SHUTDOWN_WAKE)
			continue;

		pSession = (CSession*)key;

		if (pSession == nullptr)
			continue;

		if (overlapped == nullptr)
			continue;

		if (overlapped == pSession->GetRecvOverlapPointer())
		{
			if (retval == 0 || transfrerred == 0)
			{
				int err = WSAGetLastError();
				if (err != 64 && err != 997 && err != 0 && err != 10038 && err != 1236)
				{
					/*
					* ERROR_NETNAME_DELETED(64) : TCP 곌껐 鍮�� 醫猷
					* WSA_IO_PENDING(997) : 以泥 I/O  以 猷
					* ERROR_NETWORK_UNREACHABLE(1236) : ㅽ몄 곌껐 ㅽ  以
					* linger 듭 ㅼ RST 瑜 利 � RST   곌껐 醫猷  湲곗 recv  ㅻ
					* WSAENOTSOCKET(10038) : nonsocket   耳 
					printf("WorkerThread GQCS Error %d\n", err);
					*/
				}
				pSession->CloseSocket();
			}
			else
			{
				pSession->GetRecvBuffer()->MoveWritePointer(transfrerred);

				st_Header header;

				int size;
			
				while (1)
				{
					size = pSession->GetRecvBuffer()->GetUseSize();

					//怨� ш린 Header ш린 
					if (size < sizeof(st_Header))
						break;

					pSession->GetRecvBuffer()->Peek((char*)&header, sizeof(st_Header));
					int len = header.size;//header.len;
					if (size - sizeof(st_Header) < len)
						break;

					CPacket packet;

					pSession->GetRecvBuffer()->MoveReadPointer(sizeof(st_Header));
					pSession->GetRecvBuffer()->Dequeue(packet.GetWriteBuffPtr(), header.size);
					packet.MoveWritePos(sizeof(st_Header) + header.size);
					OnRecv(pSession,header.type, packet);
				}
				pSession->RecvPost();
			}
			pSession->DecrementIOCnt();
		}
		else if (overlapped == pSession->GetSendOverlapPointer())
		{
			pSession->LockSendQ();
			{
				pSession->GetSendBuffer()->MoveReadPointer(transfrerred);
			}
			pSession->UnLockSendQ();

			pSession->ChangeSendFlag(FALSE);

			if (retval == 0 || transfrerred == 0)
			{
				int err = WSAGetLastError();
				if (err != 64 && err != 997 && err != 0 && err != 10038 && err != 1236)
				{
					/*
					* ERROR_NETNAME_DELETED(64) : TCP 곌껐 鍮�� 醫猷
					* WSA_IO_PENDING(997) : 以泥 I/O  以 猷
					* ERROR_NETWORK_UNREACHABLE(1236) : ㅽ몄 곌껐 ㅽ  以
					* linger 듭 ㅼ RST 瑜 利 � RST   곌껐 醫猷  湲곗 recv  ㅻ
					* WSAENOTSOCKET(10038) : nonsocket   耳 
					printf("WorkerThread GQCS Error %d\n", err);
					*/
				}
				pSession->CloseSocket();
			}
			else
			{
				pSession->SendPost();
			}
			pSession->DecrementIOCnt();
		}

		if (pSession->GetIOCnt() == 0 && pSession->GetRefCnt() == 0 && pSession->GetBoolbCloseing())
		{
			CNetServer::DisConnect(pSession);
		}
	}

	g_LogThread.ILog("=== END THREAD WorkerThread ===");

	return 0;
}

bool TryChangeZone(const SESSION_HANDLE& key, int zoneID)
{
	CSession* pSession = CNetServer::GetSession(key.Handle);
	if (pSession == nullptr)
		return false;

	// 곌껐 諛 ъъ  泥댄
	if (!pSession->GetBoolConnect()) return false;
	if (pSession->GetConnectGen() != key.Gen) return false;

	if (!pSession->AddRef()) return false;

	if (!pSession->GetBoolConnect() || pSession->GetConnectGen() != key.Gen || pSession->GetBoolbCloseing())
	{
		pSession->SubRef();
		return false;
	}

	bool bRet = pSession->SetZoneID(zoneID);
	pSession->SubRef();
	return bRet;
}

bool TrySend(const SESSION_HANDLE& key, int type, CPacket* pPacket)
{
	CSession* pSession = CNetServer::GetSession(key.Handle);
	if (pSession == nullptr)
		return false;
	
	// 곌껐 諛 ъъ  泥댄
	if (!pSession->GetBoolConnect()) return false;
	if (pSession->GetConnectGen() != key.Gen) return false;

	if (!pSession->AddRef()) return false;
	
	if (!pSession->GetBoolConnect() || pSession->GetConnectGen() != key.Gen || pSession->GetBoolbCloseing())
	{
		pSession->SubRef();
		return false;
	}

	pSession->SendPacket(type, pPacket);
	pSession->SubRef();

	return true;
}

void SessionSendQEnqueue()
{
	if (s_bSessionSendEnqueueRunning.exchange(true))
		return;

	SEND_REQ sendReq;
	while (g_SendReqQueue.TryDequeue(sendReq))
	{
		CSession* pSession = CNetServer::GetSession(sendReq.SessionHandle.Handle);
		if (pSession == nullptr)
		{
			continue;
		}
		if (!pSession->GetBoolConnect())
		{
			continue;
		}
		if (pSession->GetConnectGen() != sendReq.SessionHandle.Gen)
		{
			continue;
		}
		if (pSession->AddRef())
		{
			if (!pSession->GetBoolConnect() ||
				pSession->GetConnectGen() != sendReq.SessionHandle.Gen ||
				pSession->GetBoolbCloseing())
			{
				pSession->SubRef();
				continue;
			}
			pSession->SendPacket(sendReq.type, &sendReq.packet);
			pSession->SubRef();
		}
		if (pSession->GetIOCnt() == 0 && pSession->GetRefCnt() == 0 && pSession->GetBoolbCloseing())
		{
			CNetServer::DisConnect(pSession);
		}
	}
	s_bSessionSendEnqueueRunning = false;
}

void EnqueueDisConnectReq(CSession* pSession)
{
	g_SessionCloseQueue.Enqueue(pSession);
	PostQueuedCompletionStatus(CNetServer::GetCICP(), 0, KEY_DISCONNECT_WAKE, NULL);
}

void DequeueDisConnectReq()
{
	if(s_bSessionDisConnectDequeueRunning.exchange(true))
		return;

	CSession* pSession = nullptr;
	while (g_SessionCloseQueue.TryDequeue(pSession))
	{
		pSession->CloseSocket();

		if (pSession->GetIOCnt() == 0 && pSession->GetRefCnt() == 0 && pSession->GetBoolbCloseing())
		{
			CNetServer::DisConnect(pSession);
		}
	}

	s_bSessionDisConnectDequeueRunning = false;
}

CPlayer* GetPlayer(int handle)
{
	if (handle < 0 || handle >= MAX_CONNECT_COUNT)
		return nullptr;

	return g_PlayerManager[handle];
}

CPlayer* AllocPlayer(int& outPlayerHandle)
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

void FreePlayer(CPlayer* pPlayer)
{
	int key = pPlayer->GetPlayerHandle();
	pPlayer->Clear();
	EnterCriticalSection(&g_csPlayerManager);
	{
		g_PlayerHandleManager.push_back(key);
	}
	LeaveCriticalSection(&g_csPlayerManager);
}


void CNetServer::Init()
{
	int ret = OpenServer();
	if (ret != 0)
		return;

	AcceptCnt = 0;
	AcceptKey = 0;
	InitializeCriticalSection(&cs_SessionFreeKey);
	InitializeCriticalSection(&g_csPlayerManager);

	SessionManager.resize(MAX_CONNECT_COUNT);
	for (int i = 0; i < MAX_CONNECT_COUNT; i++)
		SessionManager[i] = nullptr;
	SessionFreeKey.reserve(MAX_CONNECT_COUNT);
	for (int i = 0; i < MAX_CONNECT_COUNT; i++)
	{
		SESSION_HANDLE handle(MAX_CONNECT_COUNT - 1 - i, 0);
		SessionFreeKey.push_back(handle);
	}

	g_PlayerManager.resize(MAX_CONNECT_COUNT);
	for (int i = 0; i < MAX_CONNECT_COUNT; i++)
		g_PlayerManager[i] = nullptr;
	g_PlayerHandleManager.resize(MAX_CONNECT_COUNT);
	for (int i = 0; i < MAX_CONNECT_COUNT; i++)
	{
		g_PlayerHandleManager.push_back(MAX_CONNECT_COUNT - 1 - i);
	}
}

int CNetServer::OpenServer()
{
	g_ServerON = true;
	int ret = 0;

	WSADATA wsa;
	if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0)
	{
		return WSAGetLastError();
	}

	CICP = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, 0, OVERLAP_RUN_THREAD);
	if (CICP == NULL)
		return WSAGetLastError();

	ret = ListenSocket(Port, listen_sock);
	if (ret != 0) return ret;
	ret = ListenSocket(GmPort, gm_listen_sock);
	if (ret != 0) return ret;

	return ret;
}

int CNetServer::ListenSocket(unsigned short _port, SOCKET& out)
{
	int ret = 0;
	out = socket(AF_INET, SOCK_STREAM, 0);

	SOCKADDR_IN serveraddr;
	ZeroMemory(&serveraddr, sizeof(serveraddr));
	serveraddr.sin_family = AF_INET;
	serveraddr.sin_addr.S_un.S_addr = htonl(INADDR_ANY);

	serveraddr.sin_port = htons(_port);

	ret = bind(out, (SOCKADDR*)&serveraddr, sizeof(serveraddr));
	if (ret == SOCKET_ERROR)
		return ret;

	ret = listen(out, SOMAXCONN);
	if (ret == SOCKET_ERROR)
		return ret;

	int optval = 0;
	int optlen = sizeof(optval);
	int tmep = setsockopt(out, SOL_SOCKET, SO_SNDBUF, (char*)&optval, sizeof(optval));

	linger _linger;
	_linger.l_onoff = 1;
	_linger.l_linger = 0;
	setsockopt(out, SOL_SOCKET, SO_LINGER, (char*)&_linger, sizeof(linger));

	int nValue = 1;
	setsockopt(out, SOL_SOCKET, TCP_NODELAY, (char*)&nValue, sizeof(nValue));
	return ret;
}

CSession* CNetServer::AddSession(SOCKET sock)
{
	CSession* pSession = nullptr;
	SESSION_HANDLE key;
	LockSessionFreeKey();
	{
		if (!SessionFreeKey.empty())
		{
			key = SessionFreeKey.back();
			SessionFreeKey.pop_back();
		}
	}
	UnLockSessionFreeKey();

	if(key.Handle < 0)
		return nullptr;

	key.Gen++;
	
	if (SessionManager[key.Handle] == nullptr)
	{
		pSession = new CSession;
		SessionManager[key.Handle] = pSession;
	}
	else
	{
		pSession = SessionManager[key.Handle];
	}

	pSession->OnAcceptJoin(sock, SESSION_HANDLE(key.Handle, key.Gen));

	return pSession;
}

void CNetServer::DisConnect(CSession* pSession)
{
	// 대� 醫猷以대㈃ 臾댁
	if (!pSession->OnStartDisconnect())
		return;

	CPlayer* pPlayer = g_PlayerManager[pSession->GetConnectPlayerHandle()];
	if (pPlayer != nullptr)
	{
		pPlayer->SetRelease();
		
		ZONE_JOB z(GetTickCount(), eZONESTATUS::RELEASE, pPlayer->GetPlayerHandle(), 0, 0, 0, 0);
		g_ZoneManager.ReqJob(z, pPlayer->GetZoneID());
	}

	pSession->OnDisconnect();
	
	LockSessionFreeKey();
	{
		//g_LogServer.ILog("DisConnect Session  Handle : %d, Gen : %d", pSession->GetConnectHandle(), pSession->GetConnectGen());
		DecrementSessionCount();
		SessionFreeKey.push_back(SESSION_HANDLE(pSession->GetConnectHandle(), pSession->GetConnectGen()));
	}
	UnLockSessionFreeKey();
}

void CNetServer::ServerLog()
{
	if (LogPrintTime + LogPrintDelay > GetTickCount())
		return;
	LogPrintTime = GetTickCount();

	g_LogServer.ILog("S:%d, P:%d, TS:%d, TP:%d Proc0 : %d, Proc1 : %d, Proc2 : %d",
		ConnectSessionCount.load(), ConnectPlayerCount.load(), TotalConnectSessionCount.load(), TotalConnectPlayerCount.load(),
		ConnectProcCount[0].load(), ConnectProcCount[1].load(), ConnectProcCount[2].load());

	g_ZoneManager.Log();
}

void CNetServer::StartServer()
{
	Init();
	h_AceeptThread = (HANDLE)_beginthreadex(NULL, 0, AceeptThread, 0, 0, NULL);
	h_GmAceeptThread = (HANDLE)_beginthreadex(NULL, 0, GMAceeptThread, 0, 0, NULL);
	h_WorkerThread = new HANDLE[OVERALP_CREATE_THREAD];
	for (int i = 0; i < OVERALP_CREATE_THREAD; i++)
	{
		h_WorkerThread[i] = (HANDLE)_beginthreadex(NULL, 0, WorkerThread, 0, 0, NULL);
	}

	g_LogServer.ILog("Port : %d, CreateThread : %d, RunThread : %d, MaxConnect : %d"
	,CNetServer::Port, OVERALP_CREATE_THREAD, OVERLAP_RUN_THREAD, MAX_CONNECT_COUNT);

	CreateProcWorkerThread();
	CreateLogThread();
}

void CNetServer::ServerShutDown()
{
	g_ServerON = false;

	closesocket(listen_sock);
	closesocket(gm_listen_sock);

	for(int i = 0; i < OVERALP_CREATE_THREAD; i++)
		PostQueuedCompletionStatus(CNetServer::GetCICP(), 0, KEY_SHUTDOWN_WAKE, NULL);

	PostMessageProcThreadExit();
	PostMessageLogThreadExit();
}

void CNetServer::WiatStopServer()
{
	WaitForSingleObject(h_AceeptThread, INFINITE);
	WaitForMultipleObjects(OVERALP_CREATE_THREAD, h_WorkerThread, true, INFINITE);
	WaitProcWorkerThread();
	WaitLogThread();
}

