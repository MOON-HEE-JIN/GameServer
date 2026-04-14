#pragma once

#include "../NetWork/CSession.h"
#include "../CUtill/CPacket.h"
#include "Observer_SERVER_STUB.h"
#include "Observer_SERVER_STUB.cpp"
class CObserverPacketProc : public Stub<CSession, CPacket>
{
	// Stub을(를) 통해 상속됨
	int DO_OBSERVER_ENTER_ZONE(CSession* pTarget, CPacket& pReqPacket) override;
	int DO_OBSERVER_EXIT_ZONE(CSession* pTarget, CPacket& pReqPacket) override;
	int DO_OBSERVER_HEARTBEAT(CSession* pTarget, CPacket& pReqPacket) override;
	int DO_OBSERVER_MESSAGE(CSession* pTarget, CPacket& pReqPacket) override;
	int DO_ERROR_PACKET(CSession* pTarget, CPacket& pReqPacket) override;
	int DO_ERROR_RESULT(CSession* pTarget, int ret, int type) override;
};
