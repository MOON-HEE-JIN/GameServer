#include "Observer_PacketProc.h"
#include "../Log/CLog.h"

int CObserverPacketProc::DO_OBSERVER_ENTER_ZONE(CSession* pTarget, CPacket& pReqPacket)
{
    st_CTS_ENTER_ZONE data;
    pReqPacket >> data;

    g_LogObserver.DLog("Enter Zone Request %d", data.ID);

    return 0;
}

int CObserverPacketProc::DO_OBSERVER_EXIT_ZONE(CSession* pTarget, CPacket& pReqPacket)
{
    st_CTS_EXIT_ZONE data;
    pReqPacket >> data;

    g_LogObserver.DLog("Exit Zone Request %d", data.ID);
    return 0;
}

int CObserverPacketProc::DO_OBSERVER_HEARTBEAT(CSession* pTarget, CPacket& pReqPacket)
{
    st_CTS_HEARTBEAT data;
    pReqPacket >> data;

    g_LogObserver.DLog("HeartBeat %d", data.Number);
    return 0;
}

int CObserverPacketProc::DO_OBSERVER_MESSAGE(CSession* pTarget, CPacket& pReqPacket)
{
    st_CTS_MESSAGE data;
    pReqPacket >> data;

    g_LogObserver.DLog("Observer Message %s", data.msg.Message.msg.c_str());
    return 0;
}

int CObserverPacketProc::DO_ERROR_PACKET(CSession* pTarget, CPacket& pReqPacket)
{
    return 0;
}

int CObserverPacketProc::DO_ERROR_RESULT(CSession* pTarget, int ret, int type)
{
    return 0;
}
