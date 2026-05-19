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

public:
	int GetActiveCount() { return m_iActive.load(); }

public:
	void Init(int coordX, int coordZ, int tilew, int tileh);

	bool AddPlayer(int key, CEntity* pEntity);
	bool RemovePlayer(int key, CEntity* pEntity);

	void Update() {};
};