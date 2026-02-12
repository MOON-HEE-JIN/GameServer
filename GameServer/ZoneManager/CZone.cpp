#include "CZone.h"
#include "../NetWork/CNetServer.h"

CZone::CZone(int managerIndex, int pid, int max)

	: m_ID(managerIndex), m_ZonePid(pid), m_MaxZoneManagerCount(max)
{
	m_vecPlayer.reserve(max);
	m_Cnt.store(0);
}

CZone::~CZone()
{

}

bool CZone::EnterZone(CPlayer* pPlayer)
{
	if (pPlayer == nullptr)
		return false;

	if (m_vecPlayer.size() >= m_MaxZoneManagerCount)
		return false;

	// Player 를 못찾았으면 나가기
	if (m_mapIDtoIndex.find(pPlayer->GetPlayerHandle()) != m_mapIDtoIndex.end())
		return false;
	
	if (!TryChangePid(pPlayer->GetSessionHandle(), m_ID))
		return false;

	pPlayer->SetZoneID(m_ID);

	m_mapIDtoIndex[pPlayer->GetPlayerHandle()] = static_cast<int>(m_vecPlayer.size());
	m_vecPlayer.push_back(pPlayer);

	CNetServer::IncrementProcCount(m_ZonePid);
	m_Cnt.fetch_add(1);
	return true;
}

bool CZone::LeaveZone(CPlayer* pPlayer)
{
	if (pPlayer == nullptr || m_vecPlayer.empty())
		return false;

	std::unordered_map<int, int>::iterator iter = m_mapIDtoIndex.find(pPlayer->GetPlayerHandle());
	
	// 해당 플레이어 없으며 나가기
	if (iter == m_mapIDtoIndex.end())
		return false;

	// 마지막 Player Index
	const int leaveIndex = iter->second;
	if (leaveIndex < 0 || leaveIndex >= static_cast<int>(m_vecPlayer.size()))
		return false;

	// 마지막 플레이어 가져오기
	CPlayer* ePlayer = m_vecPlayer.back();
	
	// 마지막 플레이어 와 같지 않다면 교체
	if (ePlayer != pPlayer)
	{
		// 교체
		m_vecPlayer[leaveIndex] = ePlayer;
		m_mapIDtoIndex[ePlayer->GetPlayerHandle()] = leaveIndex;
	}

	m_vecPlayer.pop_back();
	m_mapIDtoIndex.erase(iter);
	CNetServer::DecrementProcCount(m_ZonePid);
	m_Cnt.fetch_sub(1);
	return true;
}


