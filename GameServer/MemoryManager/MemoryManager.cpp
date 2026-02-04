#include "MemoryManager.h"


CLockFreeQueue_MPSC<PROC_MSG> g_ProcJobQueue[ProcThreadCnt];
CLockFreeQueue_MPSC<SEND_REQ> g_SendReqQueue;
CLockFreeQueue_MPSC<CSession*> g_SessionCloseQueue;
CLockFreeQueue_MPSC<LOG_JOB> g_LogJobQueue;