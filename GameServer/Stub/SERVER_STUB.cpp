#include "PacketEnumDef.h"
#include "SERVER_STUB.h"

template<typename Object, typename Packet>
void Stub<Object,Packet>::InitRegisterFuncPointer()
{
	m_mapGAMEProc[GAME::LOOPBACK] = std::bind(&Stub::DO_GAME_LOOPBACK, this, std::placeholders::_1, std::placeholders::_2);
	m_mapGAMEProc[GAME::CHANGEPID] = std::bind(&Stub::DO_GAME_CHANGEPID, this, std::placeholders::_1, std::placeholders::_2);
}
template<typename Object, typename Packet>
void Stub<Object,Packet>::DO_GAME_Proc(int type, Object* pTarget, Packet& cPacket)
{
	int ret = 0;
	if(m_mapGAMEProc.find(type) == m_mapGAMEProc.end())
	{
		ret = DO_ERROR_PACKET(pTarget, cPacket);
		return;
	}
	ret = m_mapGAMEProc[type](pTarget, cPacket);
	if(ret != 0)		DO_ERROR_RESULT(pTarget, ret, type);
}
