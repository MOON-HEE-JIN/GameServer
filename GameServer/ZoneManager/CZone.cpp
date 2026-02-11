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
	if (pPlayer == nullptr)
		return false;

	if (m_mapIDtoIndex.find(pPlayer->GetPlayerHandle()) != m_mapIDtoIndex.end())
		return false;

	m_mapIDtoIndex[pPlayer->GetPlayerHandle()] = static_cast<int>(m_vecPlayer.size());
}

	if (pPlayer == nullptr || m_vecPlayer.empty())
		return false;

	const int leaveIndex = iter->second;
	if (leaveIndex < 0 || leaveIndex >= static_cast<int>(m_vecPlayer.size()))
		return false;

	CPlayer* ePlayer = m_vecPlayer.back();
		//  ġ Player ġ ٲٱ
		m_vecPlayer[leaveIndex] = ePlayer;
		m_mapIDtoIndex[ePlayer->GetPlayerHandle()] = leaveIndex;

		return false;

	pPlayer->SetZoneID(m_ID);

	m_mapIDtoIndex[pPlayer->GetPlayerHandle()] = m_vecPlayer.size();
	m_vecPlayer.push_back(pPlayer);

	CNetServer::IncrementProcCount(m_ZonePid);
	m_Cnt.fetch_add(1);
	return true;
}

bool CZone::LeaveZone(CPlayer* pPlayer)
{
	std::unordered_map<int, int>::iterator iter = m_mapIDtoIndex.find(pPlayer->GetPlayerHandle());
	
	// 해당 Zone 에 Player 없음
	if (iter == m_mapIDtoIndex.end())
		return false;

	// 끝자리에 있는 Player
	CPlayer* ePlayer = m_vecPlayer[m_vecPlayer.size() - 1];

	// 지워야 할 Player 가 끝자리 가 아니라면 바꿔주기
	if (ePlayer != pPlayer)
	{
		// 마지막 위차 Player 위치 바꾸기
		m_vecPlayer[iter->second] = ePlayer;
		m_mapIDtoIndex[ePlayer->GetPlayerHandle()] = iter->second;
	}

	m_vecPlayer.pop_back();
	m_mapIDtoIndex.erase(iter);
	CNetServer::DecrementProcCount(m_ZonePid);
	m_Cnt.fetch_sub(1);
	return true;
}

