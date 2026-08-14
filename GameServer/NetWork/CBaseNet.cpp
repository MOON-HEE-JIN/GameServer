#include "CBaseNet.h"

#include <process.h>
#include <ws2tcpip.h>
#include <iostream>
#include<atomic>

#include "NetWorkDefine.h"
#include "../ProcWorkerThread.h"
#include "../Log/CLog.h"

#define HEADER_SIZE sizeof(st_Header)

#ifdef __DEBUG__
#define SESSION_DELETE_DELAY 5 * 1000
#else
#define SESSION_DELETE_DELAY 1 * 1000
#endif // __DEBUG__


CBaseNet::CBaseNet()
{
	m_bRun.store(false);
	m_slisten = INVALID_SOCKET;
	CICP = NULL;
	m_hAceeptThread = NULL;
	m_hWorkerThread = nullptr;
	m_hSessionFreeThread = NULL;
	m_hSessionFreeEvent = NULL;
	m_bCriticalSectionsInitialized = false;
	m_bWsaInitialized = false;

	m_iConnectSessionCount.store(0);

	m_iRecvOverlappedCount.store(0);
	m_iSendOverlapeedCount.store(0);
	m_iRecvOverlappedSize.store(0);
	m_iSendOverlappedSize.store(0);
}

CBaseNet::~CBaseNet()
{
	if (m_bRun.load())
		ServerShutDown();

	if (m_hAceeptThread != NULL || m_hWorkerThread != nullptr || m_hSessionFreeThread != NULL)
		WaitStopServer();

	for (CSession* pSession : m_vecSessionManager)
		delete pSession;
	m_vecSessionManager.clear();

	if (m_hSessionFreeEvent != NULL)
	{
		CloseHandle(m_hSessionFreeEvent);
		m_hSessionFreeEvent = NULL;
	}

	if (CICP != NULL)
	{
		CloseHandle(CICP);
		CICP = NULL;
	}

	if (m_slisten != INVALID_SOCKET)
	{
		closesocket(m_slisten);
		m_slisten = INVALID_SOCKET;
	}

	if (m_bCriticalSectionsInitialized)
	{
		DeleteCriticalSection(&cs_SessionFreeKey);
		DeleteCriticalSection(&cs_SessionFree);
		m_bCriticalSectionsInitialized = false;
	}

	if (m_bWsaInitialized)
	{
		WSACleanup();
		m_bWsaInitialized = false;
	}
}

int CBaseNet::Init(int Port, int RunWorkerThreadCount)
{
	int ret = 0;

	m_Port = Port;
	m_iRunWorkerThreadCount = RunWorkerThreadCount;

	WSADATA wsa;
	if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0)
	{
		return WSAGetLastError();
	}
	m_bWsaInitialized = true;

	CICP = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, 0, OVERLAP_RUN_THREAD);
	if (CICP == NULL)
		return WSAGetLastError();

	ret = ListenSocket(m_Port, m_slisten);
	if (ret != 0)
		return ret;

	InitializeCriticalSection(&cs_SessionFreeKey);
	InitializeCriticalSection(&cs_SessionFree);
	m_bCriticalSectionsInitialized = true;

	m_hSessionFreeEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
	if (m_hSessionFreeEvent == NULL)
		return GetLastError();

	m_vecSessionManager.resize(MAX_CONNECT_COUNT);
	for (int i = 0; i < MAX_CONNECT_COUNT; i++)
		m_vecSessionManager[i] = nullptr;
	m_vecSessionFreeKey.reserve(MAX_CONNECT_COUNT);
	for (int i = 0; i < MAX_CONNECT_COUNT; i++)
	{
		SESSION_HANDLE handle(MAX_CONNECT_COUNT - 1 - i, 0);
		m_vecSessionFreeKey.push_back(handle);
	}


	return ret;
}

int CBaseNet::ListenSocket(unsigned short _port, SOCKET& out)
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

CSession* CBaseNet::OnSessionAccept(SOCKET sock)
{
	CSession* pSession = nullptr;
	SESSION_HANDLE key;
	LockSessionFreeKey();
	{
		if (!m_vecSessionFreeKey.empty())
		{
			key = m_vecSessionFreeKey.back();
			m_vecSessionFreeKey.pop_back();
		}
	}
	UnLockSessionFreeKey();

	if (key.Handle < 0)
		return nullptr;

	key.Gen++;

	if (m_vecSessionManager[key.Handle] == nullptr)
	{
		pSession = new CSession;
		m_vecSessionManager[key.Handle] = pSession;
	}
	else
	{
		pSession = m_vecSessionManager[key.Handle];
	}

	pSession->OnAcceptJoin(sock, SESSION_HANDLE(key.Handle, key.Gen));
	m_iConnectSessionCount.fetch_add(1);
	return pSession;
}

void CBaseNet::OnRecv(CSession* pSession, int type, CPacket& packet)
{
	
}

void CBaseNet::PushSessionFree(CSession* pSession)
{
	EnterCriticalSection(&cs_SessionFree);
	{
		if(pSession->TryPushFreeVector())
			m_vecSessionFree.push(pSession);
	}
	LeaveCriticalSection(&cs_SessionFree);

	// 이미 큐에 있더라도 I/O/Ref가 마지막으로 해제된 시점에 Free Thread를 깨운다.
	SetEvent(m_hSessionFreeEvent);
}

void CBaseNet::TrySessionFree(CSession* pSession)
{
	if (pSession == nullptr || !pSession->CanQueueFree())
		return;

	PushSessionFree(pSession);
}

void CBaseNet::DisConnect(CSession* pSession)
{
	if (pSession == nullptr)
		return;

	// 먼저 추가 I/O와 Ref 획득을 차단하고, 모든 사용이 끝난 뒤에만 재사용 큐로 보낸다.
	pSession->CloseSocket();
	TrySessionFree(pSession);
}

void CBaseNet::Recv(CSession* pSession, int type, CPacket& packet)
{
	if (pSession == nullptr)
		return;

	OnRecv(pSession, type, packet);
}

CSession* CBaseNet::GetSession(const SESSION_HANDLE& key)
{
	if (key.Handle < 0 || key.Handle >= m_vecSessionManager.size())
		return nullptr;
	return m_vecSessionManager[key.Handle];
}

unsigned __stdcall CBaseNet::AceeptThread(void* arg)
{
	CBaseNet* pThis = static_cast<CBaseNet*>(arg);
	pThis->AcceptRun();

	return 0;
}

int CBaseNet::AcceptRun()
{
	int ret;
	int addrlen;
	SOCKADDR_IN clientaddr;
	SOCKET client_sock;
	HANDLE IOretval;

	while (m_bRun.load())
	{
		addrlen = sizeof(clientaddr);
		client_sock = accept(m_slisten, (SOCKADDR*)&clientaddr, &addrlen);
	
		if (client_sock == INVALID_SOCKET)
		{
			ret = GetLastError();
			g_LogServer.ELog("[Accept Error = %d", ret);
			continue;
		}
		CSession* pSession = OnSessionAccept(client_sock);
		if (pSession == nullptr)
		{
			closesocket(client_sock);
			continue;
		}

		IOretval = CreateIoCompletionPort((HANDLE)client_sock, CICP, (ULONG_PTR)pSession, 0);

		if (IOretval == NULL)
		{
			DisConnect(pSession);
			g_LogServer.ELog("[Accept CICP Error = %d", GetLastError());
			continue;
		}

		if (!OnClientJoin(pSession))
		{
			DisConnect(pSession);
			continue;
		}

		if (!pSession->RecvPost())
			DisConnect(pSession);
	}
	return 0;
}

unsigned __stdcall CBaseNet::WorkerThread(void* arg)
{
	CBaseNet* pThis = static_cast<CBaseNet*>(arg);
	pThis->WorkerRun();
	return 0;
}

int CBaseNet::WorkerRun()
{
#ifdef __DEBUG__
	static LONG d_DiscountCount = 0;
	static LONG d_RecvDiscountCount = 0;
	static LONG d_SendDiscountCount = 0;
#endif // __DEBUG__

	int ret;
	CSession* pSession = nullptr;
	DWORD transfrerred;

	ULONG_PTR pKey;
	while (m_bRun.load())
	{
		pSession = nullptr;
		transfrerred = 0;
		OVERLAPPED* overlapped = nullptr;
		ret = GetQueuedCompletionStatus(CICP, &transfrerred, &pKey, &overlapped, INFINITE);

		if (pKey == KEY_SHUTDOWN_WAKE)
			continue;

		pSession = (CSession*)pKey;

		if (pSession == nullptr || overlapped == nullptr)
			continue;

		if (overlapped == pSession->GetRecvOverlapPointer())
		{
			if (ret == 0 || transfrerred == 0)
			{
				int err = GetLastError();
				if (err != 64 && err != 997 && err != 0 && err != 10038 && err != 1236)
				{
					/*
					* ERROR_NETNAME_DELETED(64) : TCP 연결이 비정상적 종료
					* WSA_IO_PENDING(997) : 중첩 I/O 작업 나중에 완료
					* ERROR_NETWORK_UNREACHABLE(1236) : 네트워크 연결이 시스템에 의해 중단
					* linger 옵션이 설정시 RST 를 즉시 전송 RST 에의 해 연결이 종료 되어 대기중인 recv 에서 오류
					* WSAENOTSOCKET(10038) : nonsocket 에 대한 소켓 작업
					printf("WorkerThread GQCS Error %d\n", err);
					*/
					g_LogServer.ELog("[Recv GQCS Error = %d]", err);
				}
#ifdef __DEBUG__
				InterlockedAdd(&d_RecvDiscountCount, 1);
#endif // __DEBUG__
				pSession->CloseSocket();
			}
			else
			{
				m_iRecvOverlappedCount.fetch_add(1);
				m_iRecvOverlappedSize.fetch_add(transfrerred);
				pSession->GetRecvBuffer()->MoveWritePointer(transfrerred);

				st_Header header;

				int size;

				while (1)
				{
					size = pSession->GetRecvBuffer()->GetUseSize();

					if (size < HEADER_SIZE)
						break;

					pSession->GetRecvBuffer()->Peek((char*)&header, HEADER_SIZE);
					int len = header.size;
					if (size - HEADER_SIZE < len)
						break;

					CPacket packet;

					pSession->GetRecvBuffer()->MoveReadPointer(HEADER_SIZE);
					pSession->GetRecvBuffer()->Dequeue(packet.GetWriteBuffPtr(), header.size);
					packet.MoveWritePos(HEADER_SIZE + header.size);
					Recv(pSession, header.type, packet);
				}
				pSession->RecvPost();
			}
			pSession->DecrementIOCnt();
		}
		else if (overlapped == pSession->GetSendOverlapPointer())
		{
			m_iSendOverlapeedCount.fetch_add(1);
			m_iSendOverlappedSize.fetch_add(transfrerred);

			pSession->LockSendQ();
			{
				pSession->GetSendBuffer()->MoveReadPointer(transfrerred);
			}
			pSession->UnLockSendQ();

			pSession->ChangeSendFlag(FALSE);

			if (ret == 0 || transfrerred == 0)
			{
				int err = WSAGetLastError();
				if (err != 64 && err != 997 && err != 0 && err != 10038 && err != 1236)
				{
					/*
					* ERROR_NETNAME_DELETED(64) : TCP 연결이 비정상적 종료
					* WSA_IO_PENDING(997) : 중첩 I/O 작업 나중에 완료
					* ERROR_NETWORK_UNREACHABLE(1236) : 네트워크 연결이 시스템에 의해 중단
					* linger 옵션이 설정시 RST 를 즉시 전송 RST 에의 해 연결이 종료 되어 대기중인 recv 에서 오류
					* WSAENOTSOCKET(10038) : nonsocket 에 대한 소켓 작업
					printf("WorkerThread GQCS Error %d\n", err);
					*/
					g_LogServer.ELog("[Send GQCS Error = %d]", err);
				}
#ifdef __DEBUG__
				InterlockedAdd(&d_SendDiscountCount, 1);
#endif // __DEBUG__
				pSession->CloseSocket();
			}
			else
			{
				pSession->SendPost();
			}
			pSession->DecrementIOCnt();
		}

		TrySessionFree(pSession);
#ifdef __DEBUG__
		if (pSession->CanQueueFree())
			InterlockedAdd(&d_DiscountCount, 1);
#endif // __DEBUG__
	}

	return 0;
}

unsigned __stdcall CBaseNet::SessionFreeThread(void* arg)
{
	CBaseNet* pThis = static_cast<CBaseNet*>(arg);
	pThis->SessionFreeRun();
	return 0;
}

int CBaseNet::SessionFreeRun()
{
	while (m_bRun.load())
	{
		CSession* pSession = nullptr;
		DWORD nWaitTime = INFINITE;

		EnterCriticalSection(&cs_SessionFree);
		{
			if (!m_vecSessionFree.empty())
			{
				CSession* pFront = m_vecSessionFree.front();
				ULONGLONG nElapsed = GetTickCount64() - pFront->GetFreeTime();
				if (nElapsed >= SESSION_DELETE_DELAY && pFront->CanQueueFree())
				{
					pSession = pFront;
					m_vecSessionFree.pop();
				}
				else if (nElapsed < SESSION_DELETE_DELAY)
				{
					nWaitTime = static_cast<DWORD>(SESSION_DELETE_DELAY - nElapsed);
				}
			}
		}
		LeaveCriticalSection(&cs_SessionFree);

		if (pSession != nullptr)
		{
			SESSION_HANDLE key = pSession->GetConnectKey();

			// 사용자 객체 연결을 먼저 끊고 Session 상태를 초기화한다.
			OnDisconnect(pSession);
			pSession->OnDisconnect();

			LockSessionFreeKey();
			{
				m_vecSessionFreeKey.push_back(key);
			}
			UnLockSessionFreeKey();
			m_iConnectSessionCount.fetch_sub(1);
			continue;
		}

		WaitForSingleObject(m_hSessionFreeEvent, nWaitTime);
	}

	return 0;
}

void CBaseNet::StartServer(CBaseNet* ptr)
{
	m_bRun.store(true);

	m_hAceeptThread = (HANDLE)_beginthreadex(NULL, 0, AceeptThread, ptr, 0, NULL);
	m_hWorkerThread = new HANDLE[m_iRunWorkerThreadCount];
	for (int i = 0; i < m_iRunWorkerThreadCount; i++)
	{
		m_hWorkerThread[i] = (HANDLE)_beginthreadex(NULL, 0, WorkerThread, ptr, 0, NULL);
	}
	m_hSessionFreeThread = (HANDLE)_beginthreadex(NULL, 0, SessionFreeThread, ptr, 0, NULL);
}

void CBaseNet::WaitStopServer()
{
	if (m_hAceeptThread != NULL)
	{
		WaitForSingleObject(m_hAceeptThread, INFINITE);
		CloseHandle(m_hAceeptThread);
		m_hAceeptThread = NULL;
	}

	if (m_hWorkerThread != nullptr)
	{
		for (int i = 0; i < m_iRunWorkerThreadCount; i++)
		{
			if (m_hWorkerThread[i] != NULL)
			{
				WaitForSingleObject(m_hWorkerThread[i], INFINITE);
				CloseHandle(m_hWorkerThread[i]);
			}
		}
		delete[] m_hWorkerThread;
		m_hWorkerThread = nullptr;
	}

	if (m_hSessionFreeThread != NULL)
	{
		WaitForSingleObject(m_hSessionFreeThread, INFINITE);
		CloseHandle(m_hSessionFreeThread);
		m_hSessionFreeThread = NULL;
	}
}

void CBaseNet::TryDisConnectSession(const SESSION_HANDLE& key)
{
	CSession* pSession = GetSession(key);
	if (pSession == nullptr || pSession->GetConnectGen() != key.Gen)
		return;

	TrySessionFree(pSession);
}

void CBaseNet::ServerShutDown()
{
	if (!m_bRun.exchange(false))
		return;

	if (m_slisten != INVALID_SOCKET)
	{
		closesocket(m_slisten);
		m_slisten = INVALID_SOCKET;
	}

	for (int i = 0; i < m_iRunWorkerThreadCount; i++)
		PostQueuedCompletionStatus(CICP, 0, KEY_SHUTDOWN_WAKE, NULL);

	SetEvent(m_hSessionFreeEvent);
}
