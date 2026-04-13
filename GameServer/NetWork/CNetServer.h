#pragma once

//#include "CMemoryPool.h"

#include <map>
#include <set>
#include <vector>

#include "CBaseNet.h"
#include "../CPlayer.h"

class CNetServer : public CBaseNet
{
public:
	CNetServer() {};
	~CNetServer() {};

private:
	std::vector<CPlayer*> g_PlayerManager;
	std::vector<int> g_PlayerHandleManager;
	CRITICAL_SECTION g_csPlayerManager;

	std::atomic<int> m_iPlayerConnectCount;

protected:
	bool OnClientJoin(CSession* pSession) override;
	void OnDisconnect(CSession* pSession) override;
	void OnRecv(CSession* pSession, int type, CPacket& packet) override;

private:
	static HANDLE h_LogThread;
public:
	int Initializer(int Port, int RunWorkerThreadCount);
	int Start();
	int Wait();
	int ShutDown();
public:
	CPlayer* GetPlayer(int handle);
	CPlayer* AllocPlayer(int& outPlayerHandle);
	void FreePlayer(CPlayer* pPlayer);

};

bool TryChangeZone(const SESSION_HANDLE& key, int zoneID);
bool TrySend(const SESSION_HANDLE& key, CPacket* pPacket);


extern CNetServer g_Net;