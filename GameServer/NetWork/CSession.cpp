#include "CSession.h"
#include <WS2tcpip.h>
#include "../Stub/StructDef.h"
#include "../Log/CLog.h"
#include "../ZoneManager/CZoneManager.h"
CSession::CSession()
{
	sock = INVALID_SOCKET;
	
	InitializeCriticalSection(&m_csSendQ);

	RecvQ = new CRingBuffer;
	SendQ = new CRingBuffer;

	RecvOverlap = { 0 };
	SendOverlap = { 0 };

	bSendFlag = false;

	bCloseing = true;
	bConnect = false;
	RefCnt = 0;
	bFreeFlag = false;
	m_iFreeTime = 0;

	IOCnt = 0;

	m_ConnectKey.store(SESSION_HANDLE(-1, 0));

	m_ConnectPlayerHandle = -1;
	m_ProcId = 0;
}

CSession::~CSession()
{
	CloseSocket();

	delete RecvQ;
	delete SendQ;
	DeleteCriticalSection(&m_csSendQ);
}

bool CSession::SetProcID(int ProcID)
{
	/*
	if (!g_ZoneManager.IsValidZoneID(zoneID))
		return false;
	*/

	m_ProcId.store(ProcID);
	
	return true;
}

bool CSession::TryPushFreeVector()
{
	if (bFreeFlag.exchange(true) == false)
	{
		m_iFreeTime = GetTickCount64();
		return true;
	}
	return false;
}

void CSession::OnAcceptJoin(SOCKET sock, SESSION_HANDLE&& key)
{
	IOCnt = 0;
	this->sock = sock;
	
	bConnect = true;
	RecvOverlap = { 0 };
	SendOverlap = { 0 };
	RecvQ->Clear();
	SendQ->Clear();

	m_ConnectKey = std::move(key);
	m_ConnectPlayerHandle = -1;
	m_ProcId = 0;
	bCloseing = false;
	RefCnt = 0;
	bFreeFlag = false;
	m_iFreeTime = 0;
}

void CSession::OnDisconnect()
{
	bSendFlag = false;

	RecvOverlap = { 0 };
	SendOverlap = { 0 };
	RecvQ->Clear();
	SendQ->Clear();
	m_ConnectPlayerHandle = -1;
	m_ProcId = 0;
	CloseSocket();
}

bool CSession::AddRef()
{
	// 종료중이 아니라면
	if(!bCloseing.load())
	{
		// 사용 증가
		RefCnt.fetch_add(1);
		if (!bCloseing.load())
		{
			return true;
		}
		// 사용 끝나고 나가기
		RefCnt.fetch_sub(1);
	}
	return false;
}

int CSession::SubRef()
{
	int ref = RefCnt.fetch_sub(1);
	if (ref <= 0)
	{
		RefCnt.fetch_add(1);
		g_LogRef.ELog("Session Handle %d ReleaseRef Error", GetConnectHandle());
		return -1;
	}

	return ref - 1;
}

void CSession::CloseSocket()
{
	bool bf = false;

	// 중복 종료 막기
	if (!bCloseing.compare_exchange_strong(bf, true))
		return;
	
	//g_LogServer.ILog("CloseSocket SessionHandle : %d", GetConnectHandle());

	if (bConnect.exchange(false))
	{
		closesocket(sock);
	}
}

void CSession::SendPacket(CPacket* _packet)
{
	EnterCriticalSection(&m_csSendQ);
	int ret = SendQ->Enqueue(_packet->GetReadBuffPtr(), _packet->GetDataSize());
	LeaveCriticalSection(&m_csSendQ);

	SendPost();
	
}

void CSession::SendPost()
{
	if (bCloseing.load())
	{
		bSendFlag.exchange(false);
		return;
	}
	bool bf = false;
	if (bSendFlag.compare_exchange_strong(bf, true) == false)
		return;

	int ret;
	EnterCriticalSection(&m_csSendQ);

	if (SendQ->GetUseSize() <= 0)
	{
		bSendFlag.exchange(false);
		LeaveCriticalSection(&m_csSendQ);
		return;
	}

	InterlockedIncrement(&IOCnt);

	ZeroMemory(&SendOverlap, sizeof(OVERLAPPED));
	
	if (SendQ->GetDirectDequeueSize() < SendQ->GetUseSize())
	{
		WSABUF wsabuf[2];
		wsabuf[0].buf = (char*)SendQ->GetReadPointer();
		wsabuf[0].len = SendQ->GetDirectDequeueSize();
		wsabuf[1].buf = (char*)SendQ->GetBuffer();
		wsabuf[1].len = SendQ->GetUseSize() - SendQ->GetDirectDequeueSize();

		ret = WSASend(sock, wsabuf, 2, 0, 0, &SendOverlap, NULL);
	}
	else
	{
		WSABUF wsabuf;
		wsabuf.buf = (char*)SendQ->GetReadPointer();
		wsabuf.len = SendQ->GetDirectDequeueSize();

		ret = WSASend(sock, &wsabuf, 1, 0, 0, &SendOverlap, NULL);
	}
	
	LeaveCriticalSection(&m_csSendQ);

	if (ret == SOCKET_ERROR)
	{
		ret = WSAGetLastError();
		if (ret != WSA_IO_PENDING)
		{
			/*
			if (ret != 10038 && ret != 10054 && ret != WSA_IO_PENDING)
				LOG_INFO("SEND_WSA_ERROR_%d\n", ret);
			*/
			InterlockedExchange((DWORD*)&bSendFlag, FALSE);
			InterlockedDecrement((DWORD*)&IOCnt);
			// 다른 완료 통지가 남아 있어도 즉시 종료 상태를 공개한다.
			// 실제 Session 재사용은 IOCnt/RefCnt가 모두 0이 된 뒤 수행된다.
			CloseSocket();
		}
		else
		{
			//printf("%d Send IO_PENDING\n", (int)sock);
		}
	}
}

bool CSession::RecvPost()
{
	if (bCloseing.load())
		return false;

	IncrementIOCnt();
	DWORD flags = 0;

	int ret = 0;


	ZeroMemory(&RecvOverlap, sizeof(OVERLAPPED));
	if (RecvQ->GetDirectEnqueueSize() < RecvQ->GetFreeSize())
	{
		WSABUF wsabuf[2];
		wsabuf[0].buf = (char*)RecvQ->GetWritePointer();
		wsabuf[0].len = RecvQ->GetDirectEnqueueSize();
		wsabuf[1].buf = (char*)RecvQ->GetBuffer();
		wsabuf[1].len = RecvQ->GetFreeSize() - wsabuf[0].len;

		ret = WSARecv(sock, wsabuf, 2, NULL, &flags, &RecvOverlap, NULL);
	}

	else
	{
		WSABUF wsabuf;
		wsabuf.buf = (char*)RecvQ->GetWritePointer();
		wsabuf.len = RecvQ->GetDirectEnqueueSize();

		ret = WSARecv(sock, &wsabuf, 1, NULL, &flags, &RecvOverlap, NULL);
	}

	if (ret == SOCKET_ERROR)
	{
		ret = WSAGetLastError();
		if (ret != WSA_IO_PENDING)
		{
			if (ret != 10054 && ret != 10053)
			{
				//printf("-- Recv WSARecv Error %d ---\n", ret);
			}
			InterlockedDecrement(&IOCnt);
			// 현재 완료 처리의 I/O 카운트가 남아 있어도 소켓은 즉시 닫아야 한다.
			CloseSocket();
			return false;
		}
	}
	return true;
}
