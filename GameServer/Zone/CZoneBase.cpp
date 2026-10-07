#include "CZoneBase.h"

#include "../NetWork/CNetServer.h"
#include "../Stub/EnumDef.h"
#include "../ZoneManager/CZoneManager.h"
#include "../Log/CLog.h"

CZoneBase::CZoneBase(int channel, int ZoneID, int ProcID, int Maximum)
	: m_iChannel(channel), m_iZoneID(ZoneID), m_iProcID(ProcID),
	m_iMaximumUser(Maximum), m_iCount(0), m_bMainWorld(false),
	m_bActive(true), m_iWidth(0), m_iHeight(0)
{
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

bool CZoneBase::TryAddCount()
{
	int count = m_iCount.load(std::memory_order_relaxed);
	while (count < m_iMaximumUser)
	{
		if (m_iCount.compare_exchange_weak( count, count + 1, std::memory_order_acq_rel, std::memory_order_relaxed))
			return true;
	}
	return false;
}

bool CZoneBase::SubCount()
{
	int count = m_iCount.load(std::memory_order_relaxed);
	while (count > 0)
	{
		if (m_iCount.compare_exchange_weak( count, count - 1, std::memory_order_acq_rel, std::memory_order_relaxed))
			return true;
	}
	return false;
}
