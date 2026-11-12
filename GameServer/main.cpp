#include "./NetWork/CNetServer.h"
#include "./NetWork/CNetChatServer.h"

#include "Log/CLog.h"
#include "ZoneManager/CZoneManager.h"
#include "./Http/CHttpApiClient.h"
#include <cstdio>

#ifdef __DEBUG__

#if __CONNECT_LOGIN_DOCKER__
CApiClient g_HttpClient("https://localhost:15080");
#else
CApiClient g_HttpClient("http://localhost:5080");
#endif // __CONNECT_LOGIN_DOCKER__

#else
CApiClient g_HttpClient("https://localhost:15080");
#endif

int main()
{
	if (g_Net.Initializer(7799, 3) != 0)
		return 1;
	if (g_ChatNet.Initializer(7800, 1) != 0)
		return 1;

	// Start world threads only after all runtime components are initialized.
	g_ZoneManager.StartMainWorld();
	
	g_Net.Start();
	g_ChatNet.Start();
	g_HttpClient.Start();
	
	
	g_Net.Wait();
	g_ChatNet.Wait();
	g_HttpClient.Wait();

	Sleep(10 * 1000);
	return 0;
}
