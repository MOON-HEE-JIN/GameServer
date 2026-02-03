#include "CSession.h"
#include <WS2tcpip.h>
#include "../Stub/StructDef.h"

CSession::CSession()
{
	sock = 0;
	
	InitializeCriticalSection(&cs);
	InitializeCriticalSection(&m_csSendQ);

	RecvQ = new RingQueue;
	SendQ = new RingQueue;

	RecvOverlap = { 0 };
	SendOverlap = { 0 };

	bSendFlag = false;
	UseFlag = true;

	bConnect = true;

	IOCnt = 0;

	m_ConnectKey.store(SESSION_HANDLE(-1, 0));

	m_ConnectPlayerHandle = -1;
	m_ProcID = 0;
}

CSession::~CSession()
{
	CloseSocket();

	delete RecvQ;
	delete SendQ;
	DeleteCriticalSection(&cs);
	DeleteCriticalSection(&m_csSendQ);
}

bool CSession::SetProcID(int procID)
{
	if (AddRef())
	{
		m_ProcID.store(procID);
		SubRef();
		return true;
	}
	return false;
}

void CSession::OnAcceptJoin(SOCKET sock, SESSION_HANDLE&& key)
{
	UseFlag = true;
	
	IOCnt = 0;
	this->sock = sock;
	
	bConnect = true;
	RecvOverlap = { 0 };
	SendOverlap = { 0 };
	RecvQ->Clear();
	SendQ->Clear();

	m_ConnectKey = std::move(key);
	m_ProcID = 0;
	bCloseing = false;
	bDisconnecting = false;
	RefCnt = 0;
}

bool CSession::OnStartDisconnect()
{
	bool bf = false;
	return bDisconnecting.compare_exchange_strong(bf, true);
}

void CSession::OnDisconnect()
{
	bSendFlag = false;
	UseFlag = false;

	RecvOverlap = { 0 };
	SendOverlap = { 0 };
	RecvQ->Clear();
	SendQ->Clear();
	m_ConnectPlayerHandle = -1;
	m_ProcID = 0;
	CloseSocket();
}

bool CSession::AddRef()
{
	// 연결 중인가
	if(!bCloseing.load())
	{
		// 사용중
		RefCnt.fetch_add(1);
		if (!bCloseing.load())
		{
			return true;
		}
		// 사용중이 아니면 감소
		RefCnt.fetch_sub(1);
	}
	return false;
}

void CSession::CloseSocket()
{
	bool bf = false;

	// 이미 종료중 이라면
	if (!bCloseing.compare_exchange_strong(bf, true))
		return;
	
	if (bConnect.exchange(false))
	{
		closesocket(sock);
	}
}

void CSession::SendPacket(int _type, CPacket* _packet)
{
	EnterCriticalSection(&m_csSendQ);
	st_Header header;
	header.type = _type;
	header.size = _packet->GetDataSize();
	int ret = SendQ->Enqueue((char*)&header, sizeof(st_Header));

	ret = SendQ->Enqueue(_packet->GetReadBuffPtr(), _packet->GetDataSize());
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
		wsabuf[0].buf = SendQ->GetReadPointer();
		wsabuf[0].len = SendQ->GetDirectDequeueSize();
		wsabuf[1].buf = SendQ->GetFirstPointer();
		wsabuf[1].len = SendQ->GetUseSize() - SendQ->GetDirectDequeueSize();
		ret = WSASend(sock, wsabuf, 2, 0, 0, &SendOverlap, NULL);
	}
	else
	{
		WSABUF wsabuf;
		wsabuf.buf = SendQ->GetReadPointer();
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
			if (InterlockedDecrement((DWORD*)&IOCnt) == 0)
			{
				CloseSocket();
			}
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
		wsabuf[0].buf = RecvQ->GetWritePointer();
		wsabuf[0].len = RecvQ->GetDirectEnqueueSize();
		wsabuf[1].buf = RecvQ->GetFirstPointer();
		wsabuf[1].len = RecvQ->GetFreeSize() - wsabuf[0].len;

		ret = WSARecv(sock, wsabuf, 2, NULL, &flags, &RecvOverlap, NULL);
	}

	else
	{
		WSABUF wsabuf;
		wsabuf.buf = RecvQ->GetWritePointer();
		wsabuf.len = RecvQ->GetDirectEnqueueSize();

		ret = WSARecv(sock, &wsabuf, 1, NULL, &flags, &RecvOverlap, NULL);
	}

	if (ret == SOCKET_ERROR)
	{
		ret = WSAGetLastError();
		if (ret != WSA_IO_PENDING)
		{
			// 10054 : 연결이 강제로 끊김, 10053 : 비정상 종료
			if (ret != 10054 && ret != 10053)
			{
				//printf("-- Recv WSARecv Error %d ---\n", ret);
			}
			if (InterlockedDecrement(&IOCnt) == 0)
			{
				CloseSocket();
			}
			return false;
		}
	}
	return true;
}