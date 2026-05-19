#pragma once
#pragma once

//#include "CMemoryPool.h"

#include <map>
#include <set>
#include <vector>

#include "CSession.h"

class CBaseNet
{
public:
	CBaseNet();
	~CBaseNet() {};

protected:
	int Init(int Port, int RunWorkerThreadCount);

private:
	int ListenSocket(unsigned short _port, SOCKET& out);
private:
	bool m_bRun;
	SOCKET m_slisten;
	HANDLE CICP;
	HANDLE m_hAceeptThread;
	HANDLE* m_hWorkerThread;
	unsigned short m_Port;
	int m_iRunWorkerThreadCount;

	std::vector<CSession*> m_vecSessionManager;
	std::vector<SESSION_HANDLE> m_vecSessionFreeKey;

	CRITICAL_SECTION cs_SessionFreeKey;

	std::atomic<int> m_iAcceptSocketCount;
	std::atomic<int> m_iConnectSessionCount;				// 현재 연결중인 세션
	std::atomic<int> m_iTotalConnectSessionCount;			// 총 연결 횟수

	
	std::atomic<int> m_iRecvOverlappedCount;
	std::atomic<int> m_iSendOverlapeedCount;
	std::atomic<int> m_iRecvOverlappedSize;
	std::atomic<int> m_iSendOverlappedSize;
protected:
	void DisConnect(CSession* pSession);
	void Recv(CSession* pSession, int type, CPacket& packet);
	
	CSession* OnSessionAccept(SOCKET sock);
	virtual bool OnClientJoin(CSession* pSession) { return true; };
	virtual void OnDisconnect(CSession* pSession) {};
	virtual void OnRecv(CSession* pSession, int type, CPacket& packet);
public:
	void LockSessionFreeKey() { EnterCriticalSection(&cs_SessionFreeKey); };
	void UnLockSessionFreeKey() { LeaveCriticalSection(&cs_SessionFreeKey); };

	bool GetRun() { return m_bRun; };
	CSession* GetSession(const SESSION_HANDLE& key);

private:
	static unsigned __stdcall AceeptThread(void* arg);		// accept() Thread
	virtual int AcceptRun();
	static unsigned __stdcall WorkerThread(void* arg);		// recv, send Thread
	virtual int WorkerRun();
protected:
	int GetRecvOverlappedCount() { return m_iRecvOverlappedCount.load(); };
	int GetSendOverlappedCount() { return m_iSendOverlapeedCount.load(); };
	int GetRecvOverlappedSize() { return m_iRecvOverlappedSize.load(); }
	int GetSendOverlappedSize() { return m_iSendOverlappedSize.load(); }
	
	void ResetRecvOverlappedLog() { m_iRecvOverlappedCount.store(0); m_iRecvOverlappedSize.store(0); }
	void ResetSendOverlappedLog() { m_iSendOverlapeedCount.store(0); m_iSendOverlappedSize.store(0); };

	void StartServer(CBaseNet* ptr);
	void WaitStopServer();
	void ServerShutDown();	
};
