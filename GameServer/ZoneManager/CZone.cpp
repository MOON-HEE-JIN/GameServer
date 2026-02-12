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

	// ?대? 議댁옱 ?쒕떎硫?
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
	
	// ?대떦 Zone ??Player ?놁쓬
	if (iter == m_mapIDtoIndex.end())
		return false;

	// ?щ씪吏?Player index
	const int leaveIndex = iter->second;
	if (leaveIndex < 0 || leaveIndex >= static_cast<int>(m_vecPlayer.size()))
		return false;

	// ?앹옄由ъ뿉 ?덈뒗 Player
	CPlayer* ePlayer = m_vecPlayer.back();
	
	// 吏?뚯빞 ??Player 媛 ?앹옄由?媛 ?꾨땲?쇰㈃ 諛붽퓭二쇨린
	if (ePlayer != pPlayer)
	{
		// 留덉?留??꾩감 Player ?꾩튂 諛붽씀湲?
		m_vecPlayer[leaveIndex] = ePlayer;
		m_mapIDtoIndex[ePlayer->GetPlayerHandle()] = leaveIndex;
	}

	m_vecPlayer.pop_back();
	m_mapIDtoIndex.erase(iter);
	CNetServer::DecrementProcCount(m_ZonePid);
	m_Cnt.fetch_sub(1);
	return true;
}


