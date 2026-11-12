#pragma once

#include <map>
#include <functional> 

namespace Rpc::ChatGame
{

template<typename Object, typename Packet>
class ChatServerToGameServerStub
{
public:
	ChatServerToGameServerStub(){InitRegisterFuncPointer();}
	virtual void DO_CHAT_Proc(int type, Object* pTarget, Packet& cPacket);
private:
	std::map<int, std::function<int(Object*, Packet&)>> m_mapCHATProc;
	void InitRegisterFuncPointer();
private:
	virtual int DO_CHAT_GROUP(Object* pTarget, Packet& pReqPacket) = 0;
	virtual int DO_CHAT_HEARTBEAT(Object* pTarget, Packet& pReqPacket) = 0;
	virtual int DO_CHAT_NORMAL(Object* pTarget, Packet& pReqPacket) = 0;
	virtual int DO_CHAT_WHISPER(Object* pTarget, Packet& pReqPacket) = 0;
	virtual int DO_ERROR_PACKET(Object* pTarget, Packet& pReqPacket) = 0;
	virtual int DO_ERROR_RESULT(Object* pTarget, int ret, int type) = 0;

};

} // namespace Rpc::ChatGame
