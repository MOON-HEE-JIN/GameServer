#pragma once
#include "CHazardMemoryPool_FromGPT.h"
#include "CLockFreeQueue_FromGPT.h"
#include "../NetWork/CSession.h"

#include "../NetWork/NetWorkDefine.h"

extern CLockFreeQueue_MPSC<PROC_MSG> g_ProcLoginJobQueue;
extern CLockFreeQueue_MPSC<PROC_MSG> g_ProcJobQueue;
extern CLockFreeQueue_MPSC<SEND_REQ> g_SendReqQueue;
extern CLockFreeQueue_MPSC<CSession*> g_SessionCloseQueue;