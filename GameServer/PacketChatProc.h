#pragma once

#include "Stub/ChatGame.ChatServerToGameServerStub.g.h"
#include "Stub/ChatGame.ChatServerToGameServerStub.g.cpp"

#include "NetWork/CSession.h"
#include "CUtill/CPacket.h"

class PacketChatProc : public Rpc::ChatGame::ChatServerToGameServerStub<CSession, CPacket>
{
	// ChatServerToGameServerStub을(를) 통해 상속됨
	int DO_CHAT_GROUP(CSession* pTarget, CPacket& pReqPacket) override;
	int DO_CHAT_HEARTBEAT(CSession* pTarget, CPacket& pReqPacket) override;
	int DO_CHAT_NORMAL(CSession* pTarget, CPacket& pReqPacket) override;
	int DO_CHAT_WHISPER(CSession* pTarget, CPacket& pReqPacket) override;
	int DO_ERROR_PACKET(CSession* pTarget, CPacket& pReqPacket) override;
	int DO_ERROR_RESULT(CSession* pTarget, int ret, int type) override;
};