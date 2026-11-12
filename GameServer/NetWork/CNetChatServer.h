#pragma once

#include "CBaseNet.h"

class CNetChatServer : public CBaseNet
{
private:
	CSession* m_pChatSession;

public:
	int Initializer(int Port, int RunWorkerThreadCount) override;

protected:
	bool OnClientJoin(CSession* pSession) override;
	void OnRecv(CSession* pSession, int type, CPacket& packet) override;
	void OnDisconnect(CSession* pSession) override;
public:
	void SendPacket(CPacket* pPacket)
	{
		if (m_pChatSession == nullptr)
			return;
		m_pChatSession->SendPacket(pPacket);
	}
};

extern CNetChatServer g_ChatNet;