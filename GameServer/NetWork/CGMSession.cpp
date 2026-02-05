#include "CGMSession.h"

#include "../Stub/StructDef.h"

CGMSession::CGMSession()
{
	m_Socket = INVALID_SOCKET;
}

CGMSession::~CGMSession()
{
	Close();
}

bool CGMSession::Attach(SOCKET socket)
{
	Close();
	m_Socket = socket;
	return m_Socket != INVALID_SOCKET;
}

void CGMSession::Close()
{
	if (m_Socket == INVALID_SOCKET)
		return;

	closesocket(m_Socket);
	m_Socket = INVALID_SOCKET;
}

bool CGMSession::RecvExact(char* buffer, int size)
{
	int received = 0;
	while (received < size)
	{
		int ret = recv(m_Socket, buffer + received, size - received, 0);
		if (ret <= 0)
			return false;
		received += ret;
	}
	return true;
}

bool CGMSession::SendExact(const char* buffer, int size)
{
	int sent = 0;
	while (sent < size)
	{
		int ret = send(m_Socket, buffer + sent, size - sent, 0);
		if (ret <= 0)
			return false;
		sent += ret;
	}
	return true;
}

bool CGMSession::RecvPacket(int& outType, CPacket& outPacket)
{
	st_Header header;
	if (!RecvExact((char*)&header, sizeof(header)))
		return false;

	if (header.size < 0 || header.size > outPacket.GetBufferSize())
		return false;

	outType = header.type;
	outPacket.Clear();
	if (header.size == 0)
		return true;

	if (!RecvExact(outPacket.GetWriteBuffPtr(), header.size))
		return false;
	outPacket.MoveWritePos(header.size);
	return true;
}

bool CGMSession::SendPacket(int type, CPacket& packet)
{
	st_Header header;
	header.type = type;
	header.size = packet.GetDataSize();

	if (!SendExact((const char*)&header, sizeof(header)))
		return false;

	if (header.size > 0)
	{
		if (!SendExact(packet.GetReadBuffPtr(), header.size))
			return false;
	}

	return true;
}
