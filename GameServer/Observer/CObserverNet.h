#pragma once

#include "../NetWork/CBaseNet.h"

class CObserverNet : public CBaseNet
{
public:
	void initializer(int Port, int RunWorkerThreadCount) { Init(Port, RunWorkerThreadCount); };
	void Start() { StartServer(this); }
	void Wait() { WaitStopServer(); }
	
protected:
	virtual bool OnClientJoin(CSession* pSession) override;
	virtual void OnRecv(CSession* pSession, int type, CPacket& packet) override;
};

extern CObserverNet g_ObserverNet;