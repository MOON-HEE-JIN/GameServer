#pragma once
#pragma comment(lib, "ws2_32")

#include <WinSock2.h>
#include "../CUtill/CPacket.h"

class CGMSession
{
public:
	CGMSession();
	~CGMSession();

	bool Attach(SOCKET socket);
	void Close();

	bool RecvPacket(int& outType, CPacket& outPacket);
	bool SendPacket(int type, CPacket& packet);

private:
	bool RecvExact(char* buffer, int size);
	bool SendExact(const char* buffer, int size);

private:
	SOCKET m_Socket;
};
