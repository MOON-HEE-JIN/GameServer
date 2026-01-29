#include "PacketProc.h"
#include "Stub/PacketEnumDef.h"
#include "NetWork/CNetServer.h"

int PacketProc::DO_GAME_LOOPBACK(CPlayer* pTarget, CPacket& pReqPacket)
{
    st_CTS_LoopBack data;
    pReqPacket >> data;

    printf("LoopBackk Recv Data : %lld\n", data.data);
    
    st_STC_LoopBack reqPacket;
    reqPacket.ret = 0;
    reqPacket.data = data.data;
    CPacket req;
    req << reqPacket;

	EnqueueSendReq(pTarget->GetSessionHandle(), GAME::LOOPBACK, &req);

    return 0;
}

int PacketProc::DO_ERROR_PACKET(CPlayer* pTarget, CPacket& pReqPacket)
{
    return 0;
}

int PacketProc::DO_ERROR_RESULT(CPlayer* pTarget, int ret, int type)
{
    return 0;
}
