#include "CZoneManager.h"

#include "../GameServerDef.h"
#include "../Log/CLog.h"
#include <sstream>
CZoneManager::CZoneManager()
{
	// 해당 생성자 는 임시 Packet 을 처리하는 ProcThread * 2 만큼 생성후 ProcThread 당 Zone 2개식 할당
	m_maxZoneCnt = ProcThreadCnt * 2;
	for (int i = 0; i < m_maxZoneCnt; i++)
	{
		CZone* pZone = new CZone(i, i % ProcThreadCnt, 2000);
		m_vecZone.push_back(pZone);
		g_LogServer.ILog("Create Zone Index : %d, ProcQ : %d, Max : %d", i, i % ProcThreadCnt, 2000);
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

	if (pPlayer == nullptr || !IsValidZoneID(zoneid))
		return false;

	if (pPlayer == nullptr)
		return false;

	int zoneID = pPlayer->GetZoneID();
	if (!IsValidZoneID(zoneID))
		return false;

	return m_vecZone[zoneID]->LeaveZone(pPlayer);
	buf.reserve(m_maxZoneCnt * 16);

		const int zoneCount = m_vecZone[i]->m_Cnt.load();

		stream << "Zone[" << i << "] : " << zoneCount << " ";
		Total += zoneCount;
	CZone* pZone = new CZone(m_maxZoneCnt, 0, 2000);
	m_vecZone.push_back(pZone);
	g_LogServer.ILog("Create Zone Index : %d, ProcQ : %d, Max : %d", m_maxZoneCnt, 0, 2000);
}

CZoneManager::~CZoneManager()
{
	// 임시 소멸자
	for (int i = 0; i <= m_maxZoneCnt; i++)
	{
		delete m_vecZone[i];
	}
}

bool CZoneManager::EnterZone(CPlayer* pPlayer, int zoneid)
{
	return m_vecZone[zoneid]->EnterZone(pPlayer);
}

bool CZoneManager::LeaveZone(CPlayer* pPlayer)
{
	return m_vecZone[pPlayer->GetZoneID()]->LeaveZone(pPlayer);
}

void CZoneManager::Log()
{
	std::string buf;
	int Total = 0;
	for (int i = 0; i < m_maxZoneCnt; i++)
	{
		std::ostringstream stream;
		stream << "Zone[" << i << "] : " << m_vecZone[i]->m_Cnt.load() << " ";
		buf.append(stream.str());
		Total += m_vecZone[i]->m_Cnt.load();
	}

	std::ostringstream stream;
	stream << "Total : " << Total;
	buf.append(stream.str());

	g_LogServer.ILog(buf.c_str());
}

CZoneManager g_ZoneManager;