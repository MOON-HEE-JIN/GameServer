#pragma once

#include <map>
#include <functional> 
template<typename Object, typename Packet>
class Stub
{
public:
	Stub(){InitRegisterFuncPointer();}
	virtual void DO_GAME_Proc(int type, Object* pTarget, Packet& cPacket);
	virtual void DO_OBSERVER_Proc(int type, Object* pTarget, Packet& cPacket);
private:
	std::map<int, std::function<int(Object*, Packet&)>> m_mapGAMEProc;
	std::map<int, std::function<int(Object*, Packet&)>> m_mapOBSERVERProc;
	void InitRegisterFuncPointer();
private:
	virtual int DO_GAME_CHANGEZONE(Object* pTarget, Packet& pReqPacket) = 0;
	virtual int DO_GAME_LOOPBACK(Object* pTarget, Packet& pReqPacket) = 0;
	virtual int DO_GAME_MOVESTART(Object* pTarget, Packet& pReqPacket) = 0;
	virtual int DO_GAME_MOVESTOP(Object* pTarget, Packet& pReqPacket) = 0;
	virtual int DO_GAME_TELEPORT(Object* pTarget, Packet& pReqPacket) = 0;
	virtual int DO_OBSERVER_CONNET_OBSERVER(Object* pTarget, Packet& pReqPacket) = 0;
	virtual int DO_ERROR_PACKET(Object* pTarget, Packet& pReqPacket) = 0;
	virtual int DO_ERROR_RESULT(Object* pTarget, int ret, int type) = 0;

};