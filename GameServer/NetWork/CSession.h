#pragma once
#pragma comment(lib, "ws2_32")

#include <WinSock2.h>
#include "../CUtill/RingQueue.h"
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

	RingQueue* SendQ;
	RingQueue* RecvQ;

	DWORD IOCnt;
	std::atomic<bool> bSendFlag;
	DWORD UseFlag;
	OVERLAPPED SendOverlap;
	OVERLAPPED RecvOverlap;

	CRITICAL_SECTION cs;
	CRITICAL_SECTION m_csSendQ;

	// 접속 종료중인지
	std::atomic<bool> bCloseing;
	// 세션의 사용 완전	종료 플래그
	std::atomic<bool> bDisconnecting;
	// 접속 상태 플래그
	std::atomic<bool> bConnect;
	std::atomic<int> RefCnt;
private:
	std::atomic<SESSION_HANDLE> m_ConnectKey;
	int m_ConnectPlayerID;	// 접속한 플레이어 ID

public:
	int IncrementIOCnt() { return InterlockedIncrement(&IOCnt); }
	int DecrementIOCnt() { return InterlockedDecrement(&IOCnt); }
	bool AddRef();
	void SubRef() { RefCnt.fetch_sub(1); }

	void ChangeSendFlag(bool b) { bSendFlag.exchange(b);}

	void LockSendQ() { EnterCriticalSection(&m_csSendQ); }
	void UnLockSendQ() { LeaveCriticalSection(&m_csSendQ); }
public:
	RingQueue* GetSendBuffer() { return SendQ; }
	RingQueue* GetRecvBuffer() { return RecvQ; }

	OVERLAPPED* GetSendOverlapPointer() { return &SendOverlap; }
	OVERLAPPED* GetRecvOverlapPointer() { return &RecvOverlap; }

	SESSION_HANDLE GetConnectKey() { return m_ConnectKey; }
	int GetConnectGen() { return m_ConnectKey.load().Gen; }
	int GetConnectID() { return m_ConnectKey.load().Handle; }
	int GetConnectPlayerID() { return m_ConnectPlayerID; }
	int GetIOCnt() { return IOCnt; }
	int GetRefCnt() { return RefCnt.load(); }

	bool GetBoolbCloseing() { return bCloseing; }
	bool GetBoolConnect() { return bConnect; }

	void SetConnectPlayerID(int playerID) { m_ConnectPlayerID = playerID; }
public:
	void OnAcceptJoin(SOCKET sock, SESSION_HANDLE&& key);
	
	bool OnStartDisconnect();
	void OnDisconnect();

public:
	void CloseSocket();
	void SendPacket(int _type, CPacket* _pPacket);
	void SendPost();
	bool RecvPost();
};
