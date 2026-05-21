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
	COORDINATE m_Coord;
	int m_iTileSizeW;
	int m_iTileSizeH;

	CTContainer<CEntity> m_Players;
	CTContainer<CEntity> m_Monsters;

	int m_iDebugLogTime;
	int m_iDebugLogDelayTime = 2 * 1000;
public:
	int GetActiveCount() { return m_iActive.load(); }

public:
	void Init(int coordX, int coordZ, int tilew, int tileh);

	bool AddPlayer(int key, CEntity* pEntity);
	bool RemovePlayer(int key, CEntity* pEntity);

	void Update();
};