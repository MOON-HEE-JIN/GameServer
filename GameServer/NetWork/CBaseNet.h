#pragma once
#pragma once

//#include "CMemoryPool.h"

#include <map>
#include <set>
#include <vector>

#include "CSession.h"
#include "../CPlayer.h"

class CBaseNet
{
public:
	CBaseNet() {};
	~CBaseNet() {};

private:
	void Init();
	int OpenServer();
	int ListenSocket(unsigned short _port, SOCKET& out);
	unsigned short Port;
	unsigned short GmPort;
private:
	SOCKET listen_sock;
	
	int AcceptCnt;

	std::vector<CSession*> SessionManager;
	std::vector<SESSION_HANDLE> SessionFreeKey;
	__int64 AcceptKey;

	unsigned __int64 AllocSessionID;

	HANDLE CICP;
	HANDLE h_AceeptThread;
	HANDLE* h_WorkerThread;
	HANDLE h_LogThread;

	CRITICAL_SECTION cs_SessionFreeKey;
private:
	std::atomic<int> ConnectSessionCount;				// 현재 연결중인 세션
	std::atomic<int> TotalConnectSessionCount;			// 총 연결 횟수
	std::atomic<int> ConnectPlayerCount;					// 현재 연결중인 플레이어
	std::atomic<int> TotalConnectPlayerCount;			// 총 연결 횟수
	std::vector<std::atomic<int>> ConnectProcCount;		// Proc 에 연결

	int LogPrintTime;					// 로그 출력 시간
	int LogPrintDelay;				// 로그 출력 딜레이 시간
public:
	void LockSessionFreeKey() { EnterCriticalSection(&cs_SessionFreeKey); };
	void UnLockSessionFreeKey() { LeaveCriticalSection(&cs_SessionFreeKey); };

	SOCKET& GetListenSocket() { return listen_sock; }
	HANDLE GetCICP() { return CICP; }

	void IncrementAcceptCnt() { AcceptCnt++; }
	CSession* AddSession(SOCKET sock);
	void DisConnect(CSession* pSession);
	CSession* GetSession(int index) { return index < 0 ? nullptr : SessionManager[index]; }

	void IncrementSessionCount() { ConnectSessionCount.fetch_add(1); TotalConnectSessionCount.fetch_add(1); }
	void DecrementSessionCount() { ConnectSessionCount.fetch_sub(1); }
	void IncrementPlayerCount() { ConnectPlayerCount.fetch_add(1); TotalConnectPlayerCount.fetch_add(1); }
	void DecrementPlayerCount() { ConnectPlayerCount.fetch_sub(1); }
	void IncrementProcCount(int index) { ConnectProcCount[index].fetch_add(1); }
	void DecrementProcCount(int index) { ConnectProcCount[index].fetch_sub(1); }

	void ServerLog();
public:
	bool g_ServerON;

	void StartServer();
	void ServerShutDown();
	void WiatStopServer();
};

static void OnRecv(CSession* pSession, int type, CPacket pPacket);
static bool OnClientJoin(CSession* pSession);

static unsigned __stdcall AceeptThread(void* arg);		// accept() Thread
static unsigned __stdcall WorkerThread(void* arg);		// recv, send Thread
static unsigned __stdcall LogThread(void* arg);			// 로그 처리 Thread

static unsigned __stdcall GMAceeptThread(void* arg);

bool TryChangeZone(const SESSION_HANDLE& key, int zoneID);
bool TrySend(const SESSION_HANDLE& key, CPacket* pPacket);
void SessionSendQEnqueue();

void EnqueueDisConnectReq(CSession* pSession);
void DequeueDisConnectReq();

CPlayer* GetPlayer(int handle);
CPlayer* AllocPlayer(int& outPlayerHandle);
void FreePlayer(CPlayer* pPlayer);

extern std::vector<CPlayer*> g_PlayerManager;
