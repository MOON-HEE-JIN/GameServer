#include "MemoryManager.h"

CLockFreeQueue_MPSC<PROC_MSG> g_ProcLoginJobQueue;
CLockFreeQueue_MPSC<PROC_MSG> g_ProcJobQueue;
CLockFreeQueue_MPSC<SEND_REQ> g_SendReqQueue;
CLockFreeQueue_MPSC<CSession*> g_SessionCloseQueue;
