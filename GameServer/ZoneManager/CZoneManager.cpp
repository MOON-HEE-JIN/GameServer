#include "CZoneManager.h"

#include "../GameServerDef.h"
#include "../Log/CLog.h"
#include "CZone_Login.h"

#include <sstream>
CZoneManager::CZoneManager()
{
	// 해당 생성자는 임시로 작성함 나중에 Zone 에 사용될 Map 완성시 수정해야함
	
	CZone* pZone = new CZone_Login(0, 0, 2000);
	m_vecZone.push_back(pZone);
	g_LogServer.ILog("Create Zone Index : %d, ProcQ : %d, Max : %d", m_maxZoneCnt, 0, 2000);

	// ProcThreadCnt == 3 [0 ~ 2]
	m_maxZoneCnt = ProcThreadCnt * 2;
	for (int i = 1; i <= m_maxZoneCnt; i++)
	{
		// 1 ~ 2 까지
		int procid = (i + 1) % (ProcThreadCnt - 1) + 1;
		CZone* pZone = new CZone(i, procid, 2000);
		m_vecZone.push_back(pZone);
		g_LogServer.ILog("Create Zone Index : %d, ProcQ : %d, Max : %d", i, procid, 2000);
	}
}

CZoneManager::~CZoneManager()
{
	// Zone 삭제
	for (int i = 0; i <= m_maxZoneCnt; i++)
	{
		delete m_vecZone[i];
	}
}

bool CZoneManager::TryEnterZone(int toZone)
{
	return m_vecZone[toZone]->TryEnterZone();
}

bool CZoneManager::ReqEnterZone(CPlayer* pPlayer, int toZone)
{
	if (!TryEnterZone(toZone))
		return false;

	m_vecZone[toZone]->EnqueueJob({ GetTickCount(), eZONESTATUS::ENTER, pPlayer->GetPlayerHandle()
		,toZone, pPlayer->GetZoneID()
		,false, false });
	pPlayer->SetZoneStatus(eZONESTATUS::LEAVE);
	return true;
}

void CZoneManager::ReqJob(ZONE_JOB& job, int zone)
{
	m_vecZone[zone]->EnqueueJob(ZONE_JOB{job});
}

int CZoneManager::InitProcZoneVector(int pid, std::vector<CZone*>& vec)
{
	int nLoop = m_vecZone.size();
	int Ret = 0;
	for (int i = 0; i < nLoop; i++)
	{
		if (m_vecZone[i]->GetPid() == pid)
		{
			vec.push_back(m_vecZone[i]);
			Ret++;
		}
	}
	return Ret;
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
