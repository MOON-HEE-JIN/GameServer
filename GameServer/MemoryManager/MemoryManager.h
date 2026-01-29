#pragma once
#include "CHazardMemoryPool_FromGPT.h"
#include "CLockFreeQueue_FromGPT.h"
#include "../NetWork/CSession.h"

#include "../NetWork/NetWorkDefine.h"
#include "../GameServerDef.h"

extern CLockFreeQueue_MPSC<PROC_MSG> g_ProcJobQueue[ProcThreadCnt];
extern CLockFreeQueue_MPSC<SEND_REQ> g_SendReqQueue;
extern CLockFreeQueue_MPSC<CSession*> g_SessionCloseQueue;