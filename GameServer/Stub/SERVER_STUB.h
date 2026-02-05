#pragma once

#include <map>
#include <functional> 
template<typename Object, typename Packet>
class Stub
{
public:
	Stub(){InitRegisterFuncPointer();}
	virtual void DO_GAME_Proc(int type, Object* pTarget, Packet& cPacket);
private:
	std::map<int, std::function<int(Object*, Packet&)>> m_mapGAMEProc;
	void InitRegisterFuncPointer();
private:
	virtual int DO_GAME_CHANGEPID(Object* pTarget, Packet& pReqPacket) = 0;
	virtual int DO_GAME_LOOPBACK(Object* pTarget, Packet& pReqPacket) = 0;
	virtual int DO_ERROR_PACKET(Object* pTarget, Packet& pReqPacket) = 0;
	virtual int DO_ERROR_RESULT(Object* pTarget, int ret, int type) = 0;

};