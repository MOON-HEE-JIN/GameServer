#pragma once
#include "Stub/SERVER_STUB.h"
#include "Stub/SERVER_STUB.cpp"
#include "NetWork/CSession.h"
#include "CPlayer.h"
#include "CUtill/CPacket.h"

class PacketProc : public Stub<CPlayer, CPacket>
{
	// Stub을(를) 통해 상속됨
	int DO_GAME_LOOPBACK(CPlayer* pTarget, CPacket& pReqPacket) override;
	int DO_ERROR_PACKET(CPlayer* pTarget, CPacket& pReqPacket) override;
	int DO_ERROR_RESULT(CPlayer* pTarget, int ret, int type) override;

	// Stub을(를) 통해 상속됨
	int DO_GAME_CHANGEPID(CPlayer* pTarget, CPacket& pReqPacket) override;
};
