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
	static unsigned short Port;
private:
	static SOCKET listen_sock;
	static int AcceptCnt;

	static std::vector<CSession*> SessionManager;
	static std::vector<SESSION_HANDLE> SessionFreeKey;
	static __int64 AcceptKey;

	static unsigned __int64 AllocSessionID;

	static HANDLE CICP;
	static HANDLE h_AceeptThread;
	static HANDLE* h_WorkerThread;

	static CRITICAL_SECTION cs_SessionFreeKey;
public:
	static void LockSessionFreeKey() { EnterCriticalSection(&cs_SessionFreeKey); };
	static void UnLockSessionFreeKey() { LeaveCriticalSection(&cs_SessionFreeKey); };

	static SOCKET& GetListenSocket() { return listen_sock; }
	static HANDLE GetCICP() { return CICP; }

	static void IncrementAcceptCnt() { AcceptCnt++; }
	static CSession* AddSession(SOCKET sock);
	static void DisConnect(CSession* pSession);
	static CSession* GetSession(int index) { return index < 0 ? nullptr : SessionManager[index]; }
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

bool TrySend(const SESSION_HANDLE& key, int type, CPacket* pPacket);
void SessionSendQEnqueue();

void EnqueueDisConnectReq(CSession* pSession);
void DequeueDisConnectReq();

CPlayer* AllocPlayer(int& outPlayerHandle);
void FreePlayer(CPlayer* pPlayer);

extern std::vector<CPlayer*> g_PlayerManager;