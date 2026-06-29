#include "CZoneManager.h"

#include "../GameServerDef.h"
#include "../Log/CLog.h"
#include "../Zone/CZone_Login.h"
#include "../Stub/PacketEnumDef.h"
#include "../Stub/EnumDef.h"

#include <sstream>

#include "../Zone/CBinZoneIdx.h"
#include "../Zone/CBinZone.h"
#include "../NetWork/CNetServer.h"

CZoneManager g_ZoneManager;

CZoneManager::CZoneManager()
{
	// 해당 생성자는 임시로 작성함 나중에 Zone 에 사용될 Map 완성시 수정해야함
	
	CZoneBasic* pZone = new CZone_Login(0, 0, 0, 8000);
	m_mapZones[0].push_back(pZone);
	g_LogServer.ILog("Create Zone Index : %d, ProcQ : %d, Max : %d", m_maxZoneCnt, 0, 8000);

	//m_vecMainWorld.resize(MAX_MAIN_WORLD_COUNT);
	for (int i = 0; i < ProcMainThreadCnt; i++)
	{
		CZoneBasic* pMainWorld = new CMainWorld(i, 1, i+1, 10000);
		pMainWorld->Init(0, 1024, 1024);
		m_mapZones[1].push_back(pMainWorld);
		g_LogServer.ILog("Create MainZone channel : %d, ProcQ : %d, Max : %d", i, i + 1, 10000);
	}
	
	// ProcThreadCnt == 4 [0 ~ 3]
	m_maxZoneCnt = (ProcThreadCnt - 1) * 2;

	for (int i = 0; i < ProcSubThreadCnt; i++)
	{
		// MainWorld 는 proc[ProcMainThreadCnt] 까지 차지
		for (int channel = 0; channel < 2; channel++)
		{
			for (int zone = 2; zone < 7; zone++)
			{
				CZoneBasic* pZone = new CZone(channel, zone, (ProcMainThreadCnt + 1) + i, 2000);
				m_mapZones[zone].push_back(pZone);
				g_LogServer.ILog("Create Zone channel : %d, ProcQ : %d, Max : %d", channel, (ProcMainThreadCnt + 1) + i, 100);
			}
		}
	}

	StartMainWorld();
}

CZoneManager::~CZoneManager()
{
	std::unordered_map<int, std::vector<CZoneBasic*>>::iterator biter = m_mapZones.begin();
	// Zone 삭제
	for (biter; biter != m_mapZones.end(); ++biter)
	{
		int nLoop = static_cast<int>(biter->second.size());
		for (int i = 0; i < nLoop; i++)
		{
			delete biter->second[i];
		}
	}

	for (int i = 0; i < MAX_MAIN_WORLD_COUNT; i++)
	{
		//delete m_vecMainWorld[i];
	}
}

bool CZoneManager::ReadZoneBinFile(const char* filepath)
{
	CBinZoneIdx binZoneIdx;
	if (!binZoneIdx.Open(filepath))
		return false;
	
	const std::vector<IDX> vecZoneIdx = binZoneIdx.GetZoneIdxVector();
	int Loop = static_cast<int>(vecZoneIdx.size());
	for (int i = 0; i < Loop; i++)
	{
		const IDX& idx = vecZoneIdx[i];
		//m_mapZoneName[idx.ZoneId] = idx.ZoneName;
		//m_mapZoneIDtoIndex[idx.ZoneId] = i;
	}
	m_vecTempZone.reserve(Loop);

	CBinZone binZone;
	if (!binZone.Open(filepath))
		return false;

	binZone.GetZoneBoundsVector();
	binZone.GetPortalVector();
	binZone.GetSpawnPointVector();
	binZone.GetTriggerVolumeVector();
	binZone.GetTriggerVolumeParamMap();

	return false;
}

bool CZoneManager::TryEnterZone(int Channel, int toZone)
{
	if (!IsValidChannelZone(Channel, toZone))
		return false;

	if (m_mapZones[toZone].size() <= Channel)
		return false;

	return m_mapZones[toZone][Channel]->TryEnterZone();
}

void CZoneManager::StartMainWorld()
{
	std::vector<CZoneBasic*> vec = m_mapZones[1];
	
	if (vec.size() != ProcMainThreadCnt)
		exit(1);

	for (int i = 0; i < ProcMainThreadCnt; i++)
	{
		CMainWorld* pMain = (CMainWorld*)vec[i];
		pMain->Start();
	}
}

void CZoneManager::SendZone(int Channel, int Zone, CPacket* pPacket, COORDINATE pivot, CPlayer* pPlayer)
{
	if (!IsValidChannelZone(Channel, Zone))
		return;

	m_mapZones[Zone][Channel]->BoradCast(pPacket, pivot, pPlayer);
}

bool CZoneManager::ReqEnterLoginZone(CPlayer* pPlayer)
{
	if (pPlayer->GetZoneID() != 0)
		return false;

	if (!IsValidChannelZone(pPlayer->GetChannel(), 0))
		return false;

	CZone_Login* pZone = (CZone_Login*)m_mapZones[0][pPlayer->GetChannel()];

	pPlayer->SetZoneStatus(eZONESTATUS::LOGIN);
	ZONE_CHANGE_JOB job(GetTickCount(), eZONESTATUS::LOGIN, pPlayer->GetID()
		, pPlayer->GetChannel(), 0
		, pPlayer->GetChannel(), 0
		, 0, 0);

	pZone->Enqueue(job);
	return true;
}

bool CZoneManager::ReqEnterZone(CPlayer* pPlayer, int Channel, int ToZone)
{
	if (!TryEnterZone(Channel, ToZone))
		return false;

	int preZone = pPlayer->GetZoneID();
	int preChannel = pPlayer->GetChannel();
	
	if (!IsValidChannelZone(preChannel, preZone))
		return false;

	CZoneBasic* pFromZone = m_mapZones[preZone][preChannel];
	CZoneBasic* pToZone = m_mapZones[ToZone][Channel];


	// 같은 Proc 에서 관리한다면
	// ReqEnterZone() 은 PacketProc 에서 ZoneChangeJobProcess() 와 같은 Thread 에서 실행
	// m_queue 에 넣는 게 아니라 여기서 처리가능 하면 바로 처리
	if (IsEqualProcZoneID(preChannel, preZone, Channel, ToZone))
	{
		bool ret = 0;
		
		if (!pFromZone->LeaveZone(pPlayer))
			return false;

		if (!pToZone->TryPush(pPlayer))
		{
			// 다시 돌아가기
			pFromZone->EnterZone(pPlayer);
			return false;
		}
		
		if (!pToZone->EnterZone(pPlayer))
			return false;
		else
		{
			st_STC_ChangeZone pack;
			pack.ret = 0;
			pack.zone = ToZone;
			pack.channel = Channel;
			pPlayer->SendPacket(pack);
		}

		return true;
	}
	else
	{
		ZONE_CHANGE_JOB job(GetTickCount(), eZONESTATUS::ENTER, pPlayer->GetID()
			, Channel, ToZone
			, pPlayer->GetChannel(), pPlayer->GetZoneID()
			, 0, 0);

		pPlayer->SetZoneStatus(eZONESTATUS::LEAVE);

		pToZone->Enqueue(job);
		return true;
	}
}

int CZoneManager::InitProcZoneVector(int pid, std::vector<CZoneBasic*>& vec)
{
	int ret = 0;
	std::unordered_map<int, std::vector<CZoneBasic*>>::iterator biter = m_mapZones.begin();
	std::unordered_map<int, std::vector<CZoneBasic*>>::iterator eiter = m_mapZones.end();
	for (biter; biter != eiter; ++biter)
	{
		int nLoop = static_cast<int>(biter->second.size());
		for (int i = 0; i < nLoop; i++)
		{
			if (biter->second[i]->GetProcID() == pid)
			{
				
				vec.push_back(biter->second[i]);
				ret++;
			}
		}
	}

	return ret;
}

bool CZoneManager::IsValidZoneID(int zoneid) const
{
	if (m_mapZones.find(zoneid) == m_mapZones.end())
		return false;
	return true;
}

bool CZoneManager::IsValidChannelZone(int Channel, int ZoneID)
{
	if (!IsValidZoneID(ZoneID))
		return false;

	if (m_mapZones[ZoneID].size() <= Channel)
		return false;

	return true;
}

bool CZoneManager::IsEqualProcZoneID(int fromChannel, int from, int toChannel, int to)
{
	int fromprocid = GetProcID(fromChannel, from);
	int toprocid = GetProcID(toChannel, to);

	return fromprocid == toprocid;
}

int CZoneManager::GetProcID(int Channel, int Zone)
{
	if (!IsValidChannelZone(Channel, Zone))
		return -1;

	return m_mapZones[Zone][Channel]->GetProcID();
}

bool CZoneManager::LeaveZone(CPlayer* pPlayer)
{
	if (pPlayer == nullptr)
		return false;
	
	int channel = pPlayer->GetChannel();
	int zoneID = pPlayer->GetZoneID();
	if (!IsValidChannelZone(channel, zoneID))
		return false;

	return m_mapZones[zoneID][channel]->LeaveZone(pPlayer);
}

void CZoneManager::PushZoneMoveVector(CEntity* pEntity)
{
	int channel = pEntity->GetChannel();
	int zone = pEntity->GetZoneID();

	if (!IsValidChannelZone(channel, zone))
		return;

	m_mapZones[zone][channel]->PushMoveVector(pEntity);
}

void CZoneManager::PopZoneMoveVector(CEntity* pEntity)
{
	int channel = pEntity->GetChannel();
	int zone = pEntity->GetZoneID();

	if (!IsValidChannelZone(channel, zone))
		return;

	CZoneBasic* pZone = (CZoneBasic*)m_mapZones[zone][channel];
	pZone->PopMoveVector(pEntity);
}

CZoneBasic* CZoneManager::GetZone(int Channel, int ZoneID)
{
	if (m_mapZones.find(ZoneID) == m_mapZones.end())
		return nullptr;

	if (m_mapZones[ZoneID].size() <= Channel)
		return nullptr;

	return m_mapZones[ZoneID][Channel];
}

void CZoneManager::Log()
{
	if (m_iLogTime + m_iLogDelayTime > GetTickCount())
		return;

	m_iLogTime = GetTickCount();

	std::unordered_map<int, std::vector<CZoneBasic*>>::iterator biter = m_mapZones.begin();
	std::unordered_map<int, std::vector<CZoneBasic*>>::iterator eiter = m_mapZones.end();
	g_LogServer.ILog("===================================================");
	for (biter; biter != eiter; ++biter)
	{
		int nLoop = static_cast<int>(biter->second.size());
		std::string  buffer;
		std::ostringstream stream;
		stream << "ZoneID[" << biter->first << "]";
		for (int i = 0; i < nLoop; i++)
		{
			stream << "Channel[" << i << "]\t";
			stream << "Count [ " << biter->second[i]->GetCurCnt() << " ]";
		}
		buffer.append(stream.str());
		g_LogServer.ILog(buffer.c_str());
	}
	g_LogServer.ILog("===================================================");
}

bool EnqueueChangeJob(int id, int zone, ZONE_CHANGE_JOB& job)
{
	if (job.type == eZONESTATUS::LEAVE)
	{
		CPlayer* pPlayer = g_Net.GetPlayer(job.handle);
		if (pPlayer == nullptr)
			return false;
		
	}
	CZoneBasic* pZone = g_ZoneManager.GetZone(id, zone);
	if (pZone == nullptr)
		return false;
	
	return pZone->Enqueue(job);
}