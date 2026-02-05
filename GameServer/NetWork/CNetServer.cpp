#include "CNetServer.h"
#include <process.h>
#include <ws2tcpip.h>
#include <iostream>
#include<atomic>

#include "NetWorkDefine.h"
#include "../ProcWorkerThread.h"
#include "../Log/CLog.h"
#include "../Stub/PacketEnumDef.h"
#include "../Stub/StructDef.h"

unsigned short CNetServer::Port = 7799;
unsigned short CNetServer::GMPort = 7800;
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
HANDLE CNetServer::h_GMAceeptThread;
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
	int pid = pSession->GetProcID();
	//g_LogServer.DLog("Enqueue Job type : %d, size : %d", type, pPacket.GetDataSize());
	g_ProcJobQueue[pid].Enqueue({pSession->GetConnectPlayerHandle(),type, pPacket});
}

bool OnClientJoin(CSession* pSession)
{
	CPlayer* pPlayer = nullptr;

	int PlayerHandle;

	pPlayer = AllocPlayer(PlayerHandle);
	if(pPlayer == nullptr)
		return false;

	CNetServer::IncrementPlayerCount();
	pPlayer->Init(pSession->GetConnectKey(), PlayerHandle, pSession->GetProcID());
	pSession->SetConnectPlayerHandle(PlayerHandle);

	g_LogServer.ILog("OnClientJoin SessionHandle : %d, PlayerHandle : %d"
		, pSession->GetConnectHandle(), PlayerHandle);

	CNetServer::IncrementProcCount(0);

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
	return 0;
}

unsigned __stdcall GMAceeptThread(void* arg)
{
	SOCKADDR_IN clientaddr;
	int addrlen;
	while (CNetServer::g_ServerON)
	{
		addrlen = sizeof(clientaddr);
		SOCKET client_sock = accept(CNetServer::GetGmListenSocket(), (SOCKADDR*)&clientaddr, &addrlen);
		if (client_sock == INVALID_SOCKET)
		{
			if (!CNetServer::g_ServerON)
				break;
			continue;
		}

		CGMSession gmSession;
		if (!gmSession.Attach(client_sock))
		{
			closesocket(client_sock);
			continue;
		}

		g_LogServer.ILog("GM Session Connected");

		while (CNetServer::g_ServerON)
		{
			int type = -1;
			CPacket packet;
			if (!gmSession.RecvPacket(type, packet))
				break;

			st_GM_STC_Result result;
			result.ret = 0;
			result.type = type;
			result.target = -1;

			if (type == GM::SHUTDOWN)
			{
				CNetServer::RequestShutdown();
			}
			else if (type == GM::KICK)
			{
				st_GM_CTS_Kick req;
				packet >> req;
				result.target = req.playerHandle;
				if (!CNetServer::TryKickPlayer(req.playerHandle))
				{
					result.ret = -1;
				}
			}
			else
			{
				result.ret = -2;
			}

			CPacket sendPacket;
			sendPacket << result;
			if (!gmSession.SendPacket(GM::RESULT, sendPacket))
				break;

			if (type == GM::SHUTDOWN)
				break;
		}

		gmSession.Close();
		g_LogServer.ILog("GM Session Disconnected");
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
					* ERROR_NETNAME_DELETED(64) : TCP 연결이 비정상적 종료
					* WSA_IO_PENDING(997) : 중첩 I/O 작업 나중에 완료
					* ERROR_NETWORK_UNREACHABLE(1236) : 네트워크 연결이 시스템에 의해 중단
					*	linger 옵션이 설정시 RST 를 즉시 전송 RST 에의 해 연결이 종료 되어 대기중인 recv 에서 오류
					* WSAENOTSOCKET(10038) : nonsocket 에 대한 소켓 작업
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

					//고정된 크기의 Header 크기 확인
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
					* ERROR_NETNAME_DELETED(64) : TCP 연결이 비정상적 종료
					* WSA_IO_PENDING(997) : 중첩 I/O 작업 나중에 완료
					* ERROR_NETWORK_UNREACHABLE(1236) : 네트워크 연결이 시스템에 의해 중단
					*	linger 옵션이 설정시 RST 를 즉시 전송 RST 에의 해 연결이 종료 되어 대기중인 recv 에서 오류
					* WSAENOTSOCKET(10038) : nonsocket 에 대한 소켓 작업
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
	return 0;
}

bool TryChangePid(const SESSION_HANDLE& key, int pid)
{
	CSession* pSession = CNetServer::GetSession(key.Handle);
	if (pSession == nullptr)
		return false;

	// 연결 및 재사용 횟수 체크
	if (!pSession->GetBoolConnect()) return false;
	if (pSession->GetConnectGen() != key.Gen) return false;

	// 사용 증가
	if (!pSession->SetProcID(pid))
		return false;
	
	return true;
}

bool TrySend(const SESSION_HANDLE& key, int type, CPacket* pPacket)
{
	CSession* pSession = CNetServer::GetSession(key.Handle);
	if (pSession == nullptr)
		return false;
	
	// 연결 및 재사용 횟수 체크
	if (!pSession->GetBoolConnect()) return false;
	if (pSession->GetConnectGen() != key.Gen) return false;

	// 사용 증가
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
	AcceptCnt = 0;
	g_ServerON = true;
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

	OpenServer();
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

	listen_sock = socket(AF_INET, SOCK_STREAM, 0);

	SOCKADDR_IN serveraddr;
	ZeroMemory(&serveraddr, sizeof(serveraddr));
	serveraddr.sin_family = AF_INET;
	serveraddr.sin_addr.S_un.S_addr = htonl(INADDR_ANY);
	
	serveraddr.sin_port = htons(Port);

	ret = bind(listen_sock, (SOCKADDR*)&serveraddr, sizeof(serveraddr));

	listen(listen_sock, SOMAXCONN);

	int optval = 0;
	int optlen = sizeof(optval);
	int tmep = setsockopt(listen_sock, SOL_SOCKET, SO_SNDBUF, (char*)&optval, sizeof(optval));

	linger _linger;
	_linger.l_onoff = 1;
	_linger.l_linger = 0;
	setsockopt(listen_sock, SOL_SOCKET, SO_LINGER, (char*)&_linger, sizeof(linger));

	int nValue = 1;
	setsockopt(listen_sock, SOL_SOCKET, TCP_NODELAY, (char*)&nValue, sizeof(nValue));
	
	getsockopt(listen_sock, SOL_SOCKET, SO_SNDBUF, (char*)&optval, &optlen);

	gm_listen_sock = socket(AF_INET, SOCK_STREAM, 0);
	SOCKADDR_IN gmserveraddr;
	ZeroMemory(&gmserveraddr, sizeof(gmserveraddr));
	gmserveraddr.sin_family = AF_INET;
	gmserveraddr.sin_addr.S_un.S_addr = htonl(INADDR_ANY);
	gmserveraddr.sin_port = htons(GMPort);
	ret = bind(gm_listen_sock, (SOCKADDR*)&gmserveraddr, sizeof(gmserveraddr));
	if (ret != 0)
		return ret;

	listen(gm_listen_sock, SOMAXCONN);
	setsockopt(gm_listen_sock, SOL_SOCKET, SO_LINGER, (char*)&_linger, sizeof(linger));
	setsockopt(gm_listen_sock, SOL_SOCKET, TCP_NODELAY, (char*)&nValue, sizeof(nValue));
	
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
	// 이미 종료중이면 무시
	if (!pSession->OnStartDisconnect())
		return;

	CPlayer* pPlayer = g_PlayerManager[pSession->GetConnectPlayerHandle()];
	if (pPlayer != nullptr)
	{
		s_ProcWorker[pSession->GetProcID()]->ReleasePlayer(pPlayer);
	}

	DecrementProcCount(pSession->GetProcID());

	pSession->OnDisconnect();
	
	LockSessionFreeKey();
	{
		g_LogServer.ILog("DisConnect Session  Handle : %d, Gen : %d", pSession->GetConnectHandle(), pSession->GetConnectGen());
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

	g_LogServer.ILog("S:%d, P:%d, TS:%d, TP:%d\nProc0 : %d, Proc1 : %d, Proc2 : %d",
		ConnectSessionCount.load(), ConnectPlayerCount.load(), TotalConnectSessionCount.load(), TotalConnectPlayerCount.load(),
		ConnectProcCount[0].load(), ConnectProcCount[1].load(), ConnectProcCount[2].load());
}

void CNetServer::StartServer()
{
	Init();

	h_AceeptThread = (HANDLE)_beginthreadex(NULL, 0, AceeptThread, 0, 0, NULL);
	h_GMAceeptThread = (HANDLE)_beginthreadex(NULL, 0, GMAceeptThread, 0, 0, NULL);

	h_WorkerThread = new HANDLE[OVERALP_CREATE_THREAD];
	for (int i = 0; i < OVERALP_CREATE_THREAD; i++)
	{
		h_WorkerThread[i] = (HANDLE)_beginthreadex(NULL, 0, WorkerThread, 0, 0, NULL);
	}

	g_LogServer.ILog("Port : %d, GM Port : %d, CreateThread : %d, RunThread : %d, MaxConnect : %d"
	,CNetServer::Port, CNetServer::GMPort, OVERALP_CREATE_THREAD, OVERLAP_RUN_THREAD, MAX_CONNECT_COUNT);

	CreateProcWorkerThread();
	CreateLogThread();
}

void CNetServer::StopServer()
{
	WaitForSingleObject(h_AceeptThread, INFINITE);
	WaitForSingleObject(h_GMAceeptThread, INFINITE);
	WaitForMultipleObjects(OVERALP_CREATE_THREAD, h_WorkerThread, true, INFINITE);
	WaitProcWorkerThread();
	WaitLogThread();
}


bool CNetServer::TryKickPlayer(int playerHandle)
{
	if (playerHandle < 0 || playerHandle >= (int)g_PlayerManager.size())
		return false;

	CPlayer* pPlayer = g_PlayerManager[playerHandle];
	if (pPlayer == nullptr)
		return false;

	SESSION_HANDLE key = pPlayer->GetSessionHandle();
	CSession* pSession = GetSession(key.Handle);
	if (pSession == nullptr)
		return false;

	if (!pSession->GetBoolConnect() || pSession->GetConnectGen() != key.Gen)
		return false;

	EnqueueDisConnectReq(pSession);
	return true;
}

void CNetServer::RequestShutdown()
{
	if (!g_ServerON)
		return;

	g_ServerON = false;
	PostMessageExit();
	closesocket(listen_sock);
	closesocket(gm_listen_sock);

	for (int i = 0; i < OVERALP_CREATE_THREAD; ++i)
	{
		PostQueuedCompletionStatus(CICP, 0, 0, NULL);
	}
}
