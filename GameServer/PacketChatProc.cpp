#include "PacketChatProc.h"

int PacketChatProc::DO_CHAT_GROUP(CSession* pTarget, CPacket& pReqPacket)
{
    return 0;
}

int PacketChatProc::DO_CHAT_HEARTBEAT(CSession* pTarget, CPacket& pReqPacket)
{
    return 0;
}

int PacketChatProc::DO_CHAT_NORMAL(CSession* pTarget, CPacket& pReqPacket)
{
    return 0;
}

int PacketChatProc::DO_CHAT_WHISPER(CSession* pTarget, CPacket& pReqPacket)
{
    return 0;
}

int PacketChatProc::DO_ERROR_PACKET(CSession* pTarget, CPacket& pReqPacket)
{
    return 0;
}

int PacketChatProc::DO_ERROR_RESULT(CSession* pTarget, int ret, int type)
{
    return 0;
}
