#include "CTile.h"

#include <Windows.h>
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
		m_iActive.fetch_add(1);
		pEntity->SetTilePos(m_Coord);
		g_LogGame.DLog("Enter Tile[%d,%d] ActiveCount : %d", m_Coord.X, m_Coord.Z, m_iActive.load());
	}
	return ret;
}

bool CTile::RemovePlayer(int key, CEntity* pEntity)
{
	bool ret = m_Players.Sub(key, pEntity);
	if (ret)
	{
		m_iActive.fetch_sub(1);
		g_LogGame.DLog("Leave Tile[%d,%d] ActiveCount : %d", m_Coord.X, m_Coord.Z, m_iActive.load());
	}
	if (!ret)
	{
		g_LogGame.ELog("ERROR LEVEL");
	}
	return ret;
}

void CTile::Update()
{
	if (m_iDebugLogTime + m_iDebugLogDelayTime < GetTickCount())
	{
		if (m_iActive.load() > 0)
		{
			g_LogGame.DLog("Log Tile[%d,%d] ActiveCount : %d", m_Coord.X, m_Coord.Z, m_iActive.load());
		}
		m_iDebugLogTime = GetTickCount();
	}
}
