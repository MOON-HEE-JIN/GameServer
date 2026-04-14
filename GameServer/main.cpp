#include "./NetWork/CNetServer.h"
#include "./Observer/CObserverNet.h"
#include "Log/CLog.h"

void main()
{
	g_Net.Initializer(7799, 3);
	g_ObserverNet.initializer(8899, 1);
	
	g_Net.Start();
	g_ObserverNet.Start();
	
	
	
	g_Net.Wait();
	g_ObserverNet.Wait();

	Sleep(10 * 1000);
}
