#pragma once

//#include "CMemoryPool.h"

#include <map>
#include <set>
#include <vector>

#include "CSession.h"
#include "../CPlayer.h"

class CNetServer
{
public:
	CNetServer() {};
	~CNetServer() {};

private:
	static void Init();
	static int OpenServer();
	static int ListenSocket(unsigned short _port, SOCKET& out);
	static unsigned short Port;
	static unsigned short GmPort;
private:
	static SOCKET listen_sock;
	static SOCKET gm_listen_sock;
	static int AcceptCnt;

	static std::vector<CSession*> SessionManager;
	static std::vector<SESSION_HANDLE> SessionFreeKey;
	static __int64 AcceptKey;

	static unsigned __int64 AllocSessionID;

	static HANDLE CICP;
	static HANDLE h_AceeptThread;
	static HANDLE h_GmAceeptThread;
	static HANDLE* h_WorkerThread;
	static HANDLE h_LogThread;

	static CRITICAL_SECTION cs_SessionFreeKey;
private:
	static std::atomic<int> ConnectSessionCount;				// 현재 연결중인 세션
	static std::atomic<int> TotalConnectSessionCount;			// 총 연결 횟수
	static std::atomic<int> ConnectPlayerCount;					// 현재 연결중인 플레이어
	static std::atomic<int> TotalConnectPlayerCount;			// 총 연결 횟수
	static std::vector<std::atomic<int>> ConnectProcCount;		// Proc 에 연결

	static int LogPrintTime;					// 로그 출력 시간
	static int LogPrintDelay;				// 로그 출력 딜레이 시간
public:
	static void LockSessionFreeKey() { EnterCriticalSection(&cs_SessionFreeKey); };
	static void UnLockSessionFreeKey() { LeaveCriticalSection(&cs_SessionFreeKey); };

	static SOCKET& GetListenSocket() { return listen_sock; }
	static SOCKET& GetGmListenSocket() { return gm_listen_sock; }
	static HANDLE GetCICP() { return CICP; }

	static void IncrementAcceptCnt() { AcceptCnt++; }
	static CSession* AddSession(SOCKET sock);
	static void DisConnect(CSession* pSession);
	static CSession* GetSession(int index) { return index < 0 ? nullptr : SessionManager[index]; }

	static void IncrementSessionCount() { ConnectSessionCount.fetch_add(1); TotalConnectSessionCount.fetch_add(1); }
	static void DecrementSessionCount() { ConnectSessionCount.fetch_sub(1); }
	static void IncrementPlayerCount() { ConnectPlayerCount.fetch_add(1); TotalConnectPlayerCount.fetch_add(1); }
	static void DecrementPlayerCount() { ConnectPlayerCount.fetch_sub(1); }
	static void IncrementProcCount(int index) { ConnectProcCount[index].fetch_add(1); }
	static void DecrementProcCount(int index) { ConnectProcCount[index].fetch_sub(1); }

	static void ServerLog();
public:
	static bool g_ServerON;

	static void StartServer();
	static void StopServer();
};

static void OnRecv(CSession* pSession,int type, CPacket pPacket);
static bool OnClientJoin(CSession* pSession);

static unsigned __stdcall AceeptThread(void* arg);		// accept() Thread
static unsigned __stdcall WorkerThread(void* arg);		// recv, send Thread
static unsigned __stdcall LogThread(void* arg);			// 로그 처리 Thread

static unsigned __stdcall GMAceeptThread(void* arg);

bool TryChangePid(const SESSION_HANDLE& key, int pid);
bool TrySend(const SESSION_HANDLE& key, int type, CPacket* pPacket);
void SessionSendQEnqueue();

void EnqueueDisConnectReq(CSession* pSession);
void DequeueDisConnectReq();

CPlayer* AllocPlayer(int& outPlayerHandle);
void FreePlayer(CPlayer* pPlayer);

extern std::vector<CPlayer*> g_PlayerManager;