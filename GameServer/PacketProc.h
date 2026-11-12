#pragma once
#include "Stub/GameClient.ClientToGameServerStub.g.h"
#include "Stub/GameClient.ClientToGameServerStub.g.cpp"
#include "NetWork/CSession.h"
#include "CPlayer.h"
#include "CUtill/CPacket.h"

using namespace Rpc::GameClient;

class PacketProc : public ClientToGameServerStub<CPlayer, CPacket>
{
	// Stub을(를) 통해 상속됨
	int DO_GAME_LOOPBACK(CPlayer* pTarget, CPacket& pReqPacket) override;
	int DO_ERROR_PACKET(CPlayer* pTarget, CPacket& pReqPacket) override;
	int DO_ERROR_RESULT(CPlayer* pTarget, int ret, int type) override;

	// Stub을(를) 통해 상속됨
	int DO_GAME_CHANGEZONE(CPlayer* pTarget, CPacket& pReqPacket) override;

	// Stub을(를) 통해 상속됨
	int DO_GAME_MOVESTART(CPlayer* pTarget, CPacket& pReqPacket) override;
	int DO_GAME_MOVESTOP(CPlayer* pTarget, CPacket& pReqPacket) override;

	// Stub을(를) 통해 상속됨
	int DO_GAME_TELEPORT(CPlayer* pTarget, CPacket& pReqPacket) override;

	// ClientToGameServerStub을(를) 통해 상속됨
	int DO_GAME_AUTHCODE(CPlayer* pTarget, CPacket& pReqPacket) override;
};
