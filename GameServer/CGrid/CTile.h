#pragma once

#include "../CEntity.h"
#include "../CUtill/CCellContainer.h"

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

	CTContainer<CEntity> m_Players;
	CTContainer<CEntity> m_Monsters;

	int m_iDebugLogTime;
	int m_iDebugLogDelayTime = 2 * 1000;
public:
	int GetActiveCount() { return m_iActive.load(); }

public:
	void Init(COORDINATE coord, st_Vector3F start, st_Vector3F end);

	bool AddPlayer(int key, CEntity* pEntity);
	bool RemovePlayer(int key, CEntity* pEntity);

	void OnReigsterGrid(int Id) { m_iManagementID = Id; }

	int GetManagementGrid() { return m_iManagementID; }
	COORDINATE GetCoord() { return m_Coord; }

	void Update();
};