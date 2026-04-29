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
	void StartServer(CBaseNet* ptr);
	void WaitStopServer();
	void ServerShutDown();	
};
