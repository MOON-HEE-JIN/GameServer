#include "CTile.h"

#include "../Log/CLog.h"
void CTile::Init(int coordX, int coordZ, int tilew, int tileh)
{
	m_Coord = { coordX, coordZ };
	
	m_iTileSizeW = tilew;
	m_iTileSizeH = tileh;

	//g_LogGame.DLog("Create Tile[%d,%d] size[%d,%d]", coordX, coordZ, tilew, tileh);
}
bool CTile::AddPlayer(int key, CEntity* pEntity)
{
	bool ret = m_Players.Add(key, pEntity);
	if (ret)
	{
		//g_LogGame.DLog("Enter Tile[%d,%d]", m_Coord.X, m_Coord.Z);
		m_iActive.fetch_add(1);
	}
	return ret;
}

bool CTile::RemovePlayer(int key, CEntity* pEntity)
{
	bool ret = m_Players.Sub(key, pEntity);
	if (ret)
	{
		//g_LogGame.DLog("Leave Tile[%d,%d]", m_Coord.X, m_Coord.Z);
		m_iActive.fetch_sub(1);
	}
	return ret;
}
