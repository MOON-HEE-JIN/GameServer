#include "CNetChatServer.h"
#include "../Log/CLog.h"
#include "../PacketChatProc.h"

CNetChatServer g_ChatNet;

static PacketChatProc s_ChatPacketProc;

int CNetChatServer::Initializer(int Port, int RunWorkerThreadCount)
{
	int ret = Init(Port, RunWorkerThreadCount);
    return 0;
}

bool CNetChatServer::OnClientJoin(CSession* pSession)
{
	if (m_pChatSession != nullptr)
		return false;
	m_pChatSession = pSession;
	g_LogChat.ILog("OnClientJoin SessionHandle : %d", pSession->GetConnectHandle());
    return true;
}

void CNetChatServer::OnRecv(CSession* pSession, int type, CPacket& packet)
{
	s_ChatPacketProc.DO_CHAT_Proc(type, pSession, packet);
}

void CNetChatServer::OnDisconnect(CSession* pSession)
{
	if (m_pChatSession == pSession)
		m_pChatSession = nullptr;
}
