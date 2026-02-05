#include "PacketProc.h"
#include "Stub/PacketEnumDef.h"
#include "NetWork/CNetServer.h"
#include "GameServerDef.h"
#include "Stub/EnumDef.h"
#include "Log/CLog.h"

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
    
    if (data.pid >= ProcThreadCnt)
        return ERROR_CODE::NOT_FIND_PID;

    if (pTarget->GetProcID() == data.pid)
        return ERROR_CODE::EQUAL_PID;

    if (!TryChangePid(pTarget->GetSessionHandle(), data.pid))
        return ERROR_CODE::NOT_FIND_PID;

    //g_LogGame.DLog("Change ProcID : %d -> %d", pTarget->GetProcID(), data.pid);
    
    pTarget->ChangeProcID(data.pid);
    
    st_STC_ChangePid req;
    req.ret = ERROR_CODE::NOT_ERROR;
    CPacket reqPacket;
    reqPacket << req;

    TrySend(pTarget->GetSessionHandle(), GAME::CHANGEPID, &reqPacket);
    return ERROR_CODE::NOT_ERROR;
}
