#pragma once
#include "../Stub/ProjectDefineStruct.h"
#include "../MemoryManager/CLockFreeQueue_SPSC.h"
#include "../NetWork/NetWorkDefine.h"
#include "../CEntity.h"
#include "CTile.h"
#include "../CUtill/CLockQueueh.h"
#include "../GameServerEnumDef.h"
#include "../CUtill/CEntityManagmentVector.h"

#include <unordered_map>

struct st_AddMsg
{
	int type;
	CEntity* pEntity;
};

class CGrid
{
public:
	CGrid();
	~CGrid();

	void Init(int width, int height, int gridsizeW, int gridsizeH, st_Vector3F origin);
private:
	int m_iWidth;
	int m_iHeight;

	int m_iGridSizeW;
	int m_iGridSizeH;

	int m_iTileSizeW;
	int m_iTileSizeH;

	st_Vector3F m_stOrigin;

	CLQueue<st_AddMsg> m_AddQueue;
	CLockFreeQueue_SPSC<PROC_MSG> m_queue;

	CEntityManagementVector m_MoveVector;
private:
	int m_iTileCountW;
	int m_iTileCountH;
	int m_iTileCount;
	CTile* m_Tiles;
private:
	void AddMsgProc();
	void MoveUpdate();
public:
	void Update(void* pMainWorld);
	void Push(PROC_MSG& msg) { m_queue.Enqueue(msg); }
	
	bool AddPlayer(CEntity* pEntity);
	bool EnqueueAddPlayer(int type, CEntity* pEntity);
	bool RemovePlayer(CEntity* pEntity);
	bool EnqueueRemovePlayer(int type, CEntity* pEntity);

	void AddMove(CEntity* pEntity);
	void RemoveMove(CEntity* pEntity);
	st_Vector3F GetCenter();
};