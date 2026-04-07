#include "CZoneManager.h"

#include "../GameServerDef.h"
#include "../Log/CLog.h"
#include "CZone_Login.h"
#include "../Stub/PacketEnumDef.h"
#include "../Stub/EnumDef.h"

#include <sstream>

#include "../Zone/CBinZoneIdx.h"
#include "../Zone/CBinZone.h"

CZoneManager::CZoneManager()
{
	// 해당 생성자는 임시로 작성함 나중에 Zone 에 사용될 Map 완성시 수정해야함
	
	CZone* pZone = new CZone_Login(0, 0, 2000, "Login");
	m_mapZone[pZone->GetID()] = pZone;

}

CZoneManager::~CZoneManager()
{
	// Zone 삭제
	std::unordered_map<int, CZone*>::iterator iter = m_mapZone.begin();
	std::unordered_map<int, CZone*>::iterator eiter = m_mapZone.end();
	for (iter; iter != eiter; iter++)
	{
		delete iter->second;
	}
}

bool CZoneManager::TryEnterZone(int toZone)
{
	if (!IsValidZoneID(toZone))
		return false;
	return m_mapZone[toZone]->TryEnterZone();
}

void CZoneManager::SendZone(int zone, CPacket* pPacket, CPlayer* pPlayer)
{
	if (!IsValidZoneID(zone))
		return;
	m_mapZone[zone]->SendBroadCast(pPacket, pPlayer);
}

bool CZoneManager::SendZoneInfo(int zone, CPlayer* pPlayer)
{
	if (!IsValidZoneID(zone))
		return false;
	return m_mapZone[zone]->SendZoneInfo(pPlayer);
}

bool CZoneManager::ReqEnterZone(CPlayer* pPlayer, int toZone)
{
	if (!TryEnterZone(toZone))
		return false;

	int preZone = pPlayer->GetZoneID();

	if (IsEqualProcZoneID(preZone, toZone))
	{
		bool ret = 0;
		
		if (!m_mapZone[preZone]->LeaveZone(pPlayer))
			return false;
		if (!m_mapZone[toZone]->PushTemp(pPlayer))
		{
			// 다시 돌아가기
			m_mapZone[preZone]->EnterZone(pPlayer);
			return false;
		}
		
		if (!m_mapZone[toZone]->EnterZone(pPlayer))
			return false;
		else
		{
			st_STC_ChangeZone pack;
			pack.ret = 0;
			pack.zone = toZone;
			
			pPlayer->SendPacket(pack);
		}

		return true;
	}
	else
	{
		m_mapZone[toZone]->EnqueueJob({ GetTickCount(), eZONESTATUS::ENTER, pPlayer->GetPlayerHandle()
			,toZone, pPlayer->GetZoneID()
			,false, false });
		pPlayer->SetZoneStatus(eZONESTATUS::LEAVE);
		return true;
	}
}

void CZoneManager::ReqJob(ZONE_JOB& job, int zone)
{
	m_mapZone[zone]->EnqueueJob(ZONE_JOB{job});
}

int CZoneManager::InitProcZoneVector(int pid, std::vector<CZone*>& vec)
{
	int nLoop = m_mapZone.size();
	int Ret = 0;
	for (int i = 0; i < nLoop; i++)
	{
		if (m_mapZone[i]->GetPid() == pid)
		{
			vec.push_back(m_mapZone[i]);
			Ret++;
		}
	}
	return Ret;
}

bool CZoneManager::IsValidZoneID(int zoneid) const
{
	return m_mapZone.find(zoneid) != m_mapZone.end();
}

bool CZoneManager::IsEqualProcZoneID(int from, int to)
{
	int fromprocid = GetProcID(from);
	int toprocid = GetProcID(to);
	return fromprocid == toprocid;
}

int CZoneManager::GetProcID(int zone)
{
	if (!IsValidZoneID(zone))
		return 0;

	return m_mapZone[zone]->GetPid();
}

bool CZoneManager::EnterZone(CPlayer* pPlayer, int zoneid)
{
	if (pPlayer == nullptr || !IsValidZoneID(zoneid))
		return false;

	return m_mapZone[zoneid]->EnterZone(pPlayer);
}

bool CZoneManager::LeaveZone(CPlayer* pPlayer)
{
	if (pPlayer == nullptr)
		return false;

	int zoneID = pPlayer->GetZoneID();
	if (!IsValidZoneID(zoneID))
		return false;

	return m_mapZone[zoneID]->LeaveZone(pPlayer);
}

void CZoneManager::PushZoneMoveVector(CEntity* pEntity)
{
	if (!IsValidZoneID(pEntity->GetZoneID()))
		return ;

	m_mapZone[pEntity->GetZoneID()]->PushMoveVector(pEntity);
}

void CZoneManager::PopZoneMoveVector(CEntity* pEntity)
{
	if (!IsValidZoneID(pEntity->GetZoneID()))
		return ;

	m_mapZone[pEntity->GetZoneID()]->PopMoveVector(pEntity);
}

void CZoneManager::Log()
{
	std::string buf;
	int size = m_mapZone.size();
	buf.reserve(size * 16);
	int Total = 0;
	for (int i = 0; i <= size; i++)
	{
		const int zoneCount = m_mapZone[i]->m_Cnt.load();
		std::ostringstream stream;
		stream << "Zone[" << i << "][" << zoneCount << "] ";
		buf.append(stream.str());
		Total += zoneCount;
	}

	std::ostringstream stream;
	stream << "Total : " << Total;
	buf.append(stream.str());

	g_LogServer.ILog(buf.c_str());
}

CZoneManager g_ZoneManager;
