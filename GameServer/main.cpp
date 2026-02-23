#include "./NetWork/CNetServer.h"
#include "Log/CLog.h"

void main()
{
	CNetServer::StartServer();

	CNetServer::WiatStopServer();
}
