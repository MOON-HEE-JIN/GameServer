#include "CZoneManager.h"

#include "../GameServerDef.h"
#include "../Log/CLog.h"
#include <sstream>
CZoneManager::CZoneManager()
{
	// ?대떦 ?앹꽦?????꾩떆 Packet ??泥섎━?섎뒗 ProcThread * 2 留뚰겮 ?앹꽦??ProcThread ??Zone 2媛쒖떇 ?좊떦
	m_maxZoneCnt = ProcThreadCnt * 2;
	for (int i = 0; i < m_maxZoneCnt; i++)
	{
		CZone* pZone = new CZone(i, i % ProcThreadCnt, 2000);
		m_vecZone.push_back(pZone);
		g_LogServer.ILog("Create Zone Index : %d, ProcQ : %d, Max : %d", i, i % ProcThreadCnt, 2000);
	}

	CZone* pZone = new CZone(m_maxZoneCnt, 0, 2000);
	m_vecZone.push_back(pZone);
	g_LogServer.ILog("Create Zone Index : %d, ProcQ : %d, Max : %d", m_maxZoneCnt, 0, 2000);
}

CZoneManager::~CZoneManager()
{
	// ?꾩떆 ?뚮㈇??
	for (int i = 0; i <= m_maxZoneCnt; i++)
	{
		delete m_vecZone[i];
	}
}

bool CZoneManager::IsValidZoneID(int zoneid) const
{
	return zoneid >= 0 && zoneid < static_cast<int>(m_vecZone.size());
}

int CZoneManager::GetProcID(int zone)
{
	if (!IsValidZoneID(zone))
		return 0;

	return m_vecZone[zone]->GetPid();
}

bool CZoneManager::EnterZone(CPlayer* pPlayer, int zoneid)
{
	if (pPlayer == nullptr || !IsValidZoneID(zoneid))
		return false;

	return m_vecZone[zoneid]->EnterZone(pPlayer);
}

bool CZoneManager::LeaveZone(CPlayer* pPlayer)
{
	if (pPlayer == nullptr)
		return false;

	int zoneID = pPlayer->GetZoneID();
	if (!IsValidZoneID(zoneID))
		return false;

	return m_vecZone[zoneID]->LeaveZone(pPlayer);
}

void CZoneManager::Log()
{
	std::string buf;
	buf.reserve(m_maxZoneCnt * 16);
	int Total = 0;
	for (int i = 0; i <= m_maxZoneCnt; i++)
	{
		const int zoneCount = m_vecZone[i]->m_Cnt.load();
		std::ostringstream stream;
		stream << "Zone[" << i << "] : " << zoneCount << " ";
		buf.append(stream.str());
		Total += zoneCount;
	}

	std::ostringstream stream;
	stream << "Total : " << Total;
	buf.append(stream.str());

	g_LogServer.ILog(buf.c_str());
}

CZoneManager g_ZoneManager;
