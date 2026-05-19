#include "CZoneBase.h"

#include "../NetWork/CNetServer.h"
#include "../Stub/EnumDef.h"
#include "../ZoneManager/CZoneManager.h"
#include "../Log/CLog.h"

CZoneBase::CZoneBase(int channel, int ZoneID, int ProcID, int Maximum)
{
	m_iChannel = channel;
	m_iZoneID = ZoneID;
	m_iProcID = ProcID;
	m_iMaximumUser = Maximum;

	m_bActive = true;
}

CZoneBase::~CZoneBase()
{
	Reset();
}


void CZoneBase::Reset()
{
	//m_vecPlayers.clear();
	//m_mapIDtoIndex.clear();
	//m_queue.Clear();

	m_iProcID = -1;

	m_bActive = false;
}

bool CZoneBase::CheckPos(st_Vector3F pos)
{
	return (0 < pos.X && pos.X < m_iWidth && 0 < pos.Z && pos.Z < m_iHeight);
}