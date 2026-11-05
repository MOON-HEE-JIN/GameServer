#include "./NetWork/CNetServer.h"
#include "./Observer/CObserverNet.h"
#include "Log/CLog.h"
#include "ZoneManager/CZoneManager.h"
#include "./Http/CHttpApiClient.h"
#include <cstdio>

CApiClient g_HttpClient("http://localhost:5080");

int main()
{
	if (g_Net.Initializer(7799, 3) != 0)
		return 1;
	g_ObserverNet.initializer(8899, 1);

	// Start world threads only after all runtime components are initialized.
	g_ZoneManager.StartMainWorld();
	
	g_Net.Start();
	g_ObserverNet.Start();
	g_HttpClient.Start();
	
	
	g_Net.Wait();
	g_ObserverNet.Wait();
	g_HttpClient.Wait();

	Sleep(10 * 1000);
	return 0;
}
