#include "PacketProc.h"
#include "Stub/PacketEnumDef.h"
#include "NetWork/CNetServer.h"
#include "GameServerDef.h"
#include "Stub/EnumDef.h"
#include "Log/CLog.h"
#include "ZoneManager/CZoneManager.h"
#include "CUtill/CUtill.h"

int PacketProc::DO_GAME_LOOPBACK(CPlayer* pTarget, CPacket& pReqPacket)
{
    if (pTarget == nullptr)
        return ERROR_CODE::NOT_FIND_PID;

    st_CTS_LoopBack data;
    pReqPacket >> data;
    
    st_STC_LoopBack pack;
    pack.ret = 0;
    pack.data = data.data;
    
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

    switch (type)
    {
    case GAME::MOVESTART:
    {
        st_STC_MoveStart e;
        e.ret = ret;
        e.pos = pTarget->GetPosition();
        pTarget->SendPacket(e);
        break;
    }
    case GAME::MOVESTOP:
    {
        st_STC_MoveStop e;
        e.ret = ret;
        e.pos = pTarget->GetPosition();
        pTarget->SendPacket(e);
        break;
    }
    default:
    {
        CPacket pack;
        // header + returnvalue
        pack << type << 4 << ret;
        pTarget->SendPacket(&pack);
        break;
    }
    }

    return 0;
}

int PacketProc::DO_GAME_CHANGEZONE(CPlayer* pTarget, CPacket& pReqPacket)
{
    if (pTarget == nullptr)
        return ERROR_CODE::NOT_FIND_PID;

    st_CTS_ChangeZone data;
    pReqPacket >> data;
    
    if (!g_ZoneManager.IsValidChannelZone(data.channel, data.zone))
        return ERROR_CODE::NOT_FIND_PID;

    const int prevZoneID = pTarget->GetZoneID();
    if (!g_ZoneManager.IsValidZoneID(prevZoneID))
        return ERROR_CODE::NOT_FIND_PID;

    if (prevZoneID == data.zone)
        return ERROR_CODE::EQUAL_PID;

    // 기존 직접 Enter -> 요청 Job 을 주는쪽으로 수정

    //  이동 실패
    if (!g_ZoneManager.ReqEnterZone(pTarget, data.channel, data.zone))
        return ERROR_CODE::NOT_FIND_PID;

    // 이동 성공 패킷은 이동이 완료되고 보낸다

    return ERROR_CODE::NOT_ERROR;
}

int PacketProc::DO_GAME_MOVESTART(CPlayer* pTarget, CPacket& pReqPacket)
{
    st_CTS_MoveStart req;
    pReqPacket >> req;

    float dist = pTarget->GetPosition().DistanceToNSquared(req.pos);
    if (dist > POSITION_TOLERANCE * POSITION_TOLERANCE)
    {
        g_LogGame.DLog("NOT EQUAL POSITION CLIENT[%.2f, %.2f] - SERVER[%.2f, %.2f]",
			req.pos.X, req.pos.Y, pTarget->GetPosition().X, pTarget->GetPosition().Y);
        return ERROR_CODE::NOT_EQUAL_POSITION;
    }
    
#ifdef __DEBUG__
	st_Vector3F dir = pTarget->GetPosition().Direction(req.goal);
    if (dir.DistanceToSquared(req.dir) > 1)
    {
        g_LogGame.DLog("NOT EQUAL DIRECTION CLIENT[%.2f, %.2f] - SERVER[%.2f, %.2f]",
            req.dir.X, req.dir.Y, dir.X, dir.Y);
    }
#endif // __DEBUG__
    int ret = pTarget->MoveStart(req.goal, req.dir);
    if (ret != 0)
        return ret;

    //g_LogGame.DLog("Player[%d] MoveStart Goal[%.2f, %.2f, %.2f] Dir[%.2f, %.2f, %.2f]",
		//pTarget->GetPlayerHandle(), req.goal.X, req.goal.Y, req.goal.Z, req.dir.X, req.dir.Y, req.dir.Z);

    st_STC_MoveStart res;
    res.ret = 0;
    res.pos = pTarget->GetPosition();
    pTarget->SendPacket(res);

    st_STC_OtherMoveStart othermove;
    othermove.ID = pTarget->GetID();
    othermove.type = pTarget->GetType();
    othermove.dir = pTarget->GetDirVector();
    othermove.pos = pTarget->GetPosition();
    
    pTarget->BroadCast(othermove);
    return 0;
}

int PacketProc::DO_GAME_MOVESTOP(CPlayer* pTarget, CPacket& pReqPacket)
{
    st_CTS_MoveStop req;
    pReqPacket >> req;
    
	float dist = pTarget->GetPosition().DistanceToNSquared(req.pos);
    if (dist > POSITION_TOLERANCE * POSITION_TOLERANCE)
    {
		g_LogGame.DLog("NOT EQUAL POSITION CLIENT[%.2f, %.2f] - SERVER[%.2f, %.2f]", req.pos.X, req.pos.Z, pTarget->GetPosition().X, pTarget->GetPosition().Y);
        //return ERROR_CODE::NOT_EQUAL_POSITION;
    }

    int ret = pTarget->MoveStop(req.pos);
    if (ret != 0)
    {
        g_LogGame.ELog("Not PopMoveVector Player[%d] MoveStop Error[%d] Goal[%.2f, %.2f, %.2f] Dir[%.2f, %.2f, %.2f]",
			pTarget->GetID(), ret, req.pos.X, req.pos.Y, req.pos.Z, pTarget->GetDirVector().X, pTarget->GetDirVector().Y, pTarget->GetDirVector().Z);
        return ret;
    }

    st_STC_MoveStop res;
    res.ret = 0;
	res.ID = pTarget->GetID();
    res.pos = pTarget->GetPosition();
    pTarget->SendPacket(res);

    pTarget->BroadCast(res);
    
    return 0;
}

int PacketProc::DO_OBSERVER_CONNET_OBSERVER(CPlayer* pTarget, CPacket& pReqPacket)
{
    return 0;
}

int PacketProc::DO_GAME_TELEPORT(CPlayer* pTarget, CPacket& pReqPacket)
{
    st_CTS_Teleport req;
    pReqPacket >> req;

    
    // 이후 에러코드 수정 해야함
    CZoneBase* pZone = g_ZoneManager.GetZone(pTarget->GetChannel(), pTarget->GetZoneID());
    if (pZone == nullptr)
        return -1;
    
    if (!pZone->CheckPos(req.pos))
        return -1;

    int ret = pTarget->Teleport(req.pos);
    if (!ret)
        return ret;
    g_LogGame.DLog("TelePort");

    st_STC_Teleport res;
	res.ret = 0;
	res.pos = pTarget->GetPosition();
	pTarget->SendPacket(res);

    return 0;
}

