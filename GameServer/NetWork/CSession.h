#pragma once
#pragma comment(lib, "ws2_32")

#include <WinSock2.h>
#include "../CUtill/CRingBuffer.h"
#include "../CUtill/CPacket.h"
#include "NetWorkDefine.h"
#include <atomic>
class CSession
{
public:
	CSession();
	~CSession();
private:
	SOCKET sock;

	CRingBuffer* SendQ;
	CRingBuffer* RecvQ;

	DWORD IOCnt;
	std::atomic<bool> bSendFlag;
	OVERLAPPED SendOverlap;
	OVERLAPPED RecvOverlap;

	CRITICAL_SECTION m_csSendQ;

	// 접속 종료중인지
	std::atomic<bool> bCloseing;
	// 접속 상태 플래그
	std::atomic<bool> bConnect;
	std::atomic<int> RefCnt;
	std::atomic<bool> bFreeFlag;
	ULONGLONG m_iFreeTime;
private:
	std::atomic<SESSION_HANDLE> m_ConnectKey;
	int m_ConnectPlayerHandle;	// 접속한 플레이어 ID
	std::atomic<int> m_ProcId;
public:
	int IncrementIOCnt() { return InterlockedIncrement(&IOCnt); }
	int DecrementIOCnt() { return InterlockedDecrement(&IOCnt); }
	bool AddRef();
	int SubRef();

	void ChangeSendFlag(bool b) { bSendFlag.exchange(b);}

	void LockSendQ() { EnterCriticalSection(&m_csSendQ); }
	void UnLockSendQ() { LeaveCriticalSection(&m_csSendQ); }
public:
	CRingBuffer* GetSendBuffer() { return SendQ; }
	CRingBuffer* GetRecvBuffer() { return RecvQ; }

	OVERLAPPED* GetSendOverlapPointer() { return &SendOverlap; }
	OVERLAPPED* GetRecvOverlapPointer() { return &RecvOverlap; }

	SESSION_HANDLE GetConnectKey() { return m_ConnectKey; }
	int GetConnectGen() { return m_ConnectKey.load().Gen; }
	int GetConnectHandle() { return m_ConnectKey.load().Handle; }
	int GetConnectPlayerHandle() { return m_ConnectPlayerHandle; }
	int GetIOCnt() { return IOCnt; }
	int GetRefCnt() { return RefCnt.load(); }
	int GetProcID() { return m_ProcId.load(); }

	bool GetBoolbCloseing() { return bCloseing; }
	bool GetBoolConnect() { return bConnect; }

	void SetConnectPlayerHandle(int playerID) { m_ConnectPlayerHandle = playerID; }
	bool SetProcID(int ProcID);

	bool TryPushFreeVector();
	bool CanQueueFree()
	{
		return bCloseing.load() && GetIOCnt() == 0 && GetRefCnt() == 0;
	}
public:
	void OnAcceptJoin(SOCKET sock, SESSION_HANDLE&& key);

	void OnDisconnect();

	ULONGLONG GetFreeTime() { return m_iFreeTime; }
public:
	void CloseSocket();
	void SendPacket(CPacket* _pPacket);
	void SendPost();
	bool RecvPost();
};

