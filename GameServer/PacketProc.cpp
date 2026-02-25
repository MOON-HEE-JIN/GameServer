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
    
    if (pTarget->GetZoneID() != data.zone && pTarget->GetZoneID() != 0)
    {
        g_LogGame.ELog("Not Equal Zone Client[%d] - Server[%d]", data.zone, pTarget->GetZoneID());
    }

    st_STC_LoopBack pack;
    pack.ret = 0;
    pack.data = data.data;
    pack.zone = pTarget->GetZoneID();
    
    pTarget->SendPacket(pack);

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

    CPacket pack;
    // header + returnvalue
    pack << type << 4 << ret;
    pTarget->SendPacket(&pack);
    return 0;
}

int PacketProc::DO_GAME_CHANGEZONE(CPlayer* pTarget, CPacket& pReqPacket)
{
    if (pTarget == nullptr)
        return ERROR_CODE::NOT_FIND_PID;

    st_CTS_ChangeZone data;
    pReqPacket >> data;

    if (!g_ZoneManager.IsValidZoneID(data.zone))
        return ERROR_CODE::NOT_FIND_PID;

    const int prevZoneID = pTarget->GetZoneID();
    if (!g_ZoneManager.IsValidZoneID(prevZoneID))
        return ERROR_CODE::NOT_FIND_PID;

    if (prevZoneID == data.zone)
        return ERROR_CODE::EQUAL_PID;

    // 기존 직접 Enter -> 요청 Job 을 주는쪽으로 수정

    //  이동 실패
    if (!g_ZoneManager.ReqEnterZone(pTarget, data.zone))
        return ERROR_CODE::NOT_FIND_PID;

    // 이동 성공 패킷은 이동이 완료되고 보낸다

    return ERROR_CODE::NOT_ERROR;
}

int PacketProc::DO_GAME_ENTERZONE(CPlayer* pTarget, CPacket& pReqPacket)
{
    st_CTS_EnterZone data;
    pReqPacket >> data;

    if (pTarget->GetZoneID() != data.zone)
        return ERROR_CODE::ZONE_ID;

    // Zone 에 대한 주위 정보 보내기 (본인 제외)
    if (!g_ZoneManager.SendZoneInfo(pTarget->GetZoneID(), pTarget))
        return ERROR_CODE::ZONE_ID;

    // Zone 에서 의 본인 정보 보내기
    st_STC_CreateChar pack;
    pack.ID = pTarget->GetPlayerHandle();
    pack.pos.X = 0;
    pack.pos.Y = 0;
    pTarget->SendPacket(pack);
    return 0;
}

