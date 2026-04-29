#include "CObserverNet.h"

#include "../Log/CLog.h"
CObserverNet g_ObserverNet;


bool CObserverNet::OnClientJoin(CSession* pSession)
{
	g_LogObserver.DLog("Connect Observer Session");
	return true;
}

void CObserverNet::OnRecv(CSession* pSession, int type, CPacket& packet)
{
	//proc.DO_OBSERVER_Proc(type, pSession, packet);
}
