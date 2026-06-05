#include "CTile.h"

#include <Windows.h>
#include "../Log/CLog.h"

void CTile::Init(COORDINATE coord, st_Vector3F start, st_Vector3F end)
{
	m_Coord = coord;

	m_StartPos = start;
	m_EndPos = end;

	//g_LogGame.DLog("Create Tile[%d,%d] size[%d,%d]", coordX, coordZ, tilew, tileh);
}
void CTile::Enqueue(CPacket* pPacket)
{
	m_queuePacket.Push(pPacket);
}
bool CTile::AddPlayer(CEntity* pEntity)
{
	bool ret = m_vecPlayer.AddEntity(pEntity);
	if (ret)
	{
		m_iActive.fetch_add(1);
		pEntity->SetTilePos(m_Coord);
		g_LogGame.DLog("Enter Tile[%d,%d] ActiveCount : %d", m_Coord.X, m_Coord.Z, m_iActive.load());
	}
	return ret;
}

bool CTile::RemovePlayer(CEntity* pEntity)
{
	bool ret = m_vecPlayer.RemoveEntity(pEntity);
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
