#include "ChatGame.PacketEnumDef.g.h"
#include "ChatGame.ChatServerToGameServerStub.g.h"

namespace Rpc::ChatGame
{

template<typename Object, typename Packet>
void ChatServerToGameServerStub<Object,Packet>::InitRegisterFuncPointer()
{
	m_mapCHATProc[CHAT::HEARTBEAT] = std::bind(&ChatServerToGameServerStub::DO_CHAT_HEARTBEAT, this, std::placeholders::_1, std::placeholders::_2);
	m_mapCHATProc[CHAT::NORMAL] = std::bind(&ChatServerToGameServerStub::DO_CHAT_NORMAL, this, std::placeholders::_1, std::placeholders::_2);
	m_mapCHATProc[CHAT::WHISPER] = std::bind(&ChatServerToGameServerStub::DO_CHAT_WHISPER, this, std::placeholders::_1, std::placeholders::_2);
	m_mapCHATProc[CHAT::GROUP] = std::bind(&ChatServerToGameServerStub::DO_CHAT_GROUP, this, std::placeholders::_1, std::placeholders::_2);
}
template<typename Object, typename Packet>
void ChatServerToGameServerStub<Object,Packet>::DO_CHAT_Proc(int type, Object* pTarget, Packet& cPacket)
{
	int ret = 0;
	if(m_mapCHATProc.find(type) == m_mapCHATProc.end())
	{
		ret = DO_ERROR_PACKET(pTarget, cPacket);
		return;
	}
	ret = m_mapCHATProc[type](pTarget, cPacket);
	if(ret != 0)		DO_ERROR_RESULT(pTarget, ret, type);
}

} // namespace Rpc::ChatGame
