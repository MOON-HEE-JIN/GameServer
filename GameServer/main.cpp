#include "./NetWork/CNetServer.h"
#include "./NetWork/CBaseNet.h"
#include "Log/CLog.h"

void main()
{
	g_Net.Initializer(7799, 3);

	g_Net.Start();

	g_Net.Wait();

	Sleep(10 * 1000);
}
