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

    if (!g_ZoneManager.LeaveZone(pTarget))
        return ERROR_CODE::NOT_FIND_PID;

    if (!g_ZoneManager.EnterZone(pTarget, data.pid))
    {
        // Zone 에 못들어가면 기존 Zone 으로 이동
        if (!g_ZoneManager.EnterZone(pTarget, prevZoneID))
        {
            g_LogServer.ELog("Zone rollback fail. PlayerHandle:%d PrevZone:%d NewZone:%d",
                pTarget->GetPlayerHandle(), prevZoneID, data.pid);
        }
       return ERROR_CODE::NOT_FIND_PID;
    }
    //g_LogGame.DLog("Change ProcID : %d -> %d", pTarget->GetZoneID(), data.pid);

    st_STC_ChangePid req;
    req.ret = ERROR_CODE::NOT_ERROR;
    CPacket reqPacket;
    reqPacket << req;

    TrySend(pTarget->GetSessionHandle(), GAME::CHANGEPID, &reqPacket);
    return ERROR_CODE::NOT_ERROR;
}

