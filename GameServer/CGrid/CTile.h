#pragma once

#include "../CEntity.h"
#include "../CUtill/CEntityManagmentVector.h"
#include "../CUtill/CLockQueueh.h"
#include "../CUtill/CPacket.h"

class CTile
{
public:
	CTile() {};
	~CTile() {};
private:
	std::atomic<int> m_iActive;
	int m_iManagementID;
	COORDINATE m_Coord;
	int m_iTileSize;

	st_Vector3F m_StartPos;
	st_Vector3F m_EndPos;

	CLQueue<CPacket*> m_queuePacket;

	CEntityVector m_vecPlayer{ EIndexType::VECTOR_INDEX_TILE };
	CEntityVector m_vecMonster{ EIndexType::VECTOR_INDEX_TILE };

	int m_iDebugLogTime;
	int m_iDebugLogDelayTime = 2 * 1000;
public:
	int GetActiveCount() { return m_iActive.load(); }
public:
	void Init(COORDINATE coord, st_Vector3F start, st_Vector3F end);

	void Enqueue(CPacket* pPacket);
	bool AddPlayer(CEntity* pEntity);
	bool RemovePlayer(CEntity* pEntity);

	void OnReigsterGrid(int Id) { m_iManagementID = Id; }

	int GetManagementGrid() { return m_iManagementID; }
	COORDINATE GetCoord() { return m_Coord; }

	void Update();
public:
	void SendPacket(CPacket* pPacket);
};