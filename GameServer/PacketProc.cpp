#include "PacketProc.h"
#include "Stub/PacketEnumDef.h"
#include "NetWork/CNetServer.h"
#include "GameServerDef.h"
#include "Stub/EnumDef.h"
#include "Log/CLog.h"
#include "ZoneManager/CZoneManager.h"

int PacketProc::DO_GAME_LOOPBACK(CPlayer* pTarget, CPacket& pReqPacket)
{
    if (pTarget == nullptr)
        return ERROR_CODE::NOT_FIND_PID;

    st_CTS_LoopBack data;
    pReqPacket >> data;
    
    st_STC_LoopBack req;
    req.ret = 0;
    req.data = data.data;
    CPacket reqPacket;
    reqPacket << req;

	TrySend(pTarget->GetSessionHandle(), GAME::LOOPBACK, &reqPacket);

    return 0;
}

int PacketProc::DO_ERROR_PACKET(CPlayer* pTarget, CPacket& pReqPacket)
{
    return 0;
}

int PacketProc::DO_ERROR_RESULT(CPlayer* pTarget, int ret, int type)
{
    if (pTarget == nullptr)
        return ERROR_CODE::NOT_FIND_PID;

    CPacket req;
    req << ret;
    TrySend(pTarget->GetSessionHandle(), type, &req);
    return 0;
}

int PacketProc::DO_GAME_CHANGEPID(CPlayer* pTarget, CPacket& pReqPacket)
{
    if (pTarget == nullptr)
        return ERROR_CODE::NOT_FIND_PID;

    st_CTS_ChangePid data;
    pReqPacket >> data;

    if (!g_ZoneManager.IsValidZoneID(data.pid))
        return ERROR_CODE::NOT_FIND_PID;

    const int prevZoneID = pTarget->GetZoneID();
    if (!g_ZoneManager.IsValidZoneID(prevZoneID))
        return ERROR_CODE::NOT_FIND_PID;

    if (prevZoneID == data.pid)
        return ERROR_CODE::EQUAL_PID;

    // 기존 직접 Enter -> 요청 Job 을 주는쪽으로 수정

    //  이동 실패
    if (!g_ZoneManager.ReqEnterZone(pTarget, data.pid))
        return ERROR_CODE::NOT_FIND_PID;

    // 이동 성공 패킷은 이동이 완료되고 보낸다

    return ERROR_CODE::NOT_ERROR;
}

