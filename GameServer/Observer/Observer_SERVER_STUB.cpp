#include "Observer_PacketEnumDef.h"
#include "Observer_SERVER_STUB.h"

template<typename Object, typename Packet>
void Stub<Object,Packet>::InitRegisterFuncPointer()
{
	m_mapOBSERVERProc[OBSERVER::ENTER_ZONE] = std::bind(&Stub::DO_OBSERVER_ENTER_ZONE, this, std::placeholders::_1, std::placeholders::_2);
	m_mapOBSERVERProc[OBSERVER::EXIT_ZONE] = std::bind(&Stub::DO_OBSERVER_EXIT_ZONE, this, std::placeholders::_1, std::placeholders::_2);
	m_mapOBSERVERProc[OBSERVER::HEARTBEAT] = std::bind(&Stub::DO_OBSERVER_HEARTBEAT, this, std::placeholders::_1, std::placeholders::_2);
	m_mapOBSERVERProc[OBSERVER::MESSAGE] = std::bind(&Stub::DO_OBSERVER_MESSAGE, this, std::placeholders::_1, std::placeholders::_2);
}
template<typename Object, typename Packet>
void Stub<Object,Packet>::DO_OBSERVER_Proc(int type, Object* pTarget, Packet& cPacket)
{
	int ret = 0;
	if(m_mapOBSERVERProc.find(type) == m_mapOBSERVERProc.end())
	{
		ret = DO_ERROR_PACKET(pTarget, cPacket);
		return;
	}
	ret = m_mapOBSERVERProc[type](pTarget, cPacket);
	if(ret != 0)		DO_ERROR_RESULT(pTarget, ret, type);
}
