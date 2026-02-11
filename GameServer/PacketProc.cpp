#include "PacketProc.h"
#include "Stub/PacketEnumDef.h"
#include "NetWork/CNetServer.h"
#include "GameServerDef.h"
#include "Stub/EnumDef.h"
#include "Log/CLog.h"
#include "ZoneManager/CZoneManager.h"

int PacketProc::DO_GAME_LOOPBACK(CPlayer* pTarget, CPacket& pReqPacket)
{
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
    CPacket req;
    req << ret;
    TrySend(pTarget->GetSessionHandle(), type, &req);
    return 0;
}

int PacketProc::DO_GAME_CHANGEPID(CPlayer* pTarget, CPacket& pReqPacket)
{
    st_CTS_ChangePid data;
    pReqPacket >> data;
    
    int ret = 0;

    if (data.pid > g_ZoneManager.GetMaxZoneCnt() || data.pid < 0)
        return ERROR_CODE::NOT_FIND_PID;

    if (pTarget->GetZoneID() == data.pid)
        return ERROR_CODE::EQUAL_PID;

    // ±âÁ¸ Zone
    ret = g_ZoneManager.LeaveZone(pTarget);
    if (ret == false)
        return ERROR_CODE::NOT_FIND_PID;
    ret = g_ZoneManager.EnterZone(pTarget, data.pid);
    if (ret == false)
        return ERROR_CODE::NOT_FIND_PID;
    //g_LogGame.DLog("Change ProcID : %d -> %d", pTarget->GetZoneID(), data.pid);

    st_STC_ChangePid req;
    req.ret = ERROR_CODE::NOT_ERROR;
    CPacket reqPacket;
    reqPacket << req;

    TrySend(pTarget->GetSessionHandle(), GAME::CHANGEPID, &reqPacket);
    return ERROR_CODE::NOT_ERROR;
}
