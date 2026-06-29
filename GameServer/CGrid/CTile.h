#pragma once

#include "../CEntity.h"
#include "../CUtill/CEntityManagmentVector.h"
#include "../CUtill/CLockQueueh.h"
#include "../CUtill/CPacket.h"

class CMainWorld;

struct st_TileJob
{
	int type;
	CEntity* pEntity;
};

struct st_TileBroadCast
{
	CEntity* pEntity;
	CPacket packet;
};

class CTile
{
public:
	CTile() {};
	~CTile() {};

private:
	CMainWorld* m_parent;

	std::atomic<int> m_iActive;
	int m_iManagementID;
	COORDINATE m_Coord;
	int m_iTileSize;

	st_Vector3F m_StartPos;
	st_Vector3F m_EndPos;

	CLQueue<st_TileJob> m_queue;
	CLQueue<st_TileBroadCast> m_queueBroadCast;

	CEntityVector m_vecPlayer{ EIndexType::VECTOR_INDEX_TILE };
	CEntityVector m_vecMonster{ EIndexType::VECTOR_INDEX_TILE };

	int m_iDebugLogTime;
	int m_iDebugLogDelayTime = 2 * 1000;

private:
	void TileJobRun();
	void TileBroadCast();

	void NotifyEntityTileEnterAOI(CEntity* pEntity);
	void Broadcast(CPacket* pPacket, CEntity* pEntity = nullptr);

public:
	void Init(CMainWorld* parent, COORDINATE coord, st_Vector3F start, st_Vector3F end);

	void EnqueueJob(int type, CEntity* pEntity);
	void EnqueueBroadCast(CEntity* pEntity, CPacket* Packet);

	bool AddPlayer(CEntity* pEntity);
	bool RemovePlayer(CEntity* pEntity);

	void OnReigsterGrid(int Id) { m_iManagementID = Id; }

	int GetActiveCount() { return m_iActive.load(); }
	int GetManagementGrid() { return m_iManagementID; }
	COORDINATE& GetCoord() { return m_Coord; }

	void Update();
};