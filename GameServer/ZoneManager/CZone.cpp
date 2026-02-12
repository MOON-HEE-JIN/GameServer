#include "CZone.h"
#include "../NetWork/CNetServer.h"
#include "CZoneManager.h"
#include "../Stub/EnumDef.h"
#include "../Stub/PacketEnumDef.h"
CZone::CZone(int managerIndex, int pid, int max)

	: m_ID(managerIndex), m_ZonePid(pid), m_MaxZoneManagerCount(max)
{
	m_vecPlayer.reserve(max);
	m_Cnt.store(0);
}

CZone::~CZone()
{

}

void CZone::Update()
{
	ZONE_JOB job;
	while (m_queue.TryDequeue(job))
	{
		CPlayer* pPlayer = GetPlayer(job.handle);
		if (pPlayer == nullptr)
			return;

		// Player 가 본인 Thread 가 아닌 곳에서 사용 해도 괜찮은것은가?
		// EnterZone(pPlayer) --> if (!TryChangePid(pPlayer->GetSessionHandle(), m_ID))
		// 1. Player 의 SESSINO_HANDLE 를 통해서 Player 와 Session 이 살아있는지 확인한다
		// 2. 해당 함수 실패는 Session 의 종료 에 의해서 연결이 끊어진 상태니 false 하면 된다 Player 건드리지 않는다
		// 3. 해당 함수 성공 시 Session, Player 살아있는 상태 만약 Enter 후 삭제 가 된다 하더라도
		//    종료 의 의해 Player 삭제 처리는 EnterZone 의 ProcWorker::m_PlayerDeleteQueue 에서 처리
		//    기존 Zone 에서 관리 를 빼주기 위해 ReqLeave 를 보내면 된다
		//
		bool bRet = false;
		ZONE_JOB req;
		switch (job.type)
		{
		case NONE:
			continue;
		case STABLE:
		{
			pPlayer->SetZoneStatus(STABLE);
		}
			break;
		case ENTER:
		{
			if (job.ack)
			{
				if (job.ret)
				{
					// 작업 성공 fromZone 에서 지우기
					req(GetTickCount(), LEAVE, job.handle, job.toZone, job.fromZone, false, false);
					st_STC_ChangePid req;
					req.ret = ERROR_CODE::NOT_ERROR;

					CPacket pReq;
					pReq << req;
					pPlayer->SendPacket(GAME::CHANGEPID, &pReq);
				}
				else
				{
					// 작업 실패 실패로 정상 작동 하게
					req(GetTickCount(), STABLE, job.handle, job.toZone, job.fromZone, false, false);

					st_STC_ChangePid req;
					req.ret = ERROR_CODE::NOT_FIND_PID;
				
					CPacket pReq;
					pReq << req;
					pPlayer->SendPacket(GAME::CHANGEPID, &pReq);
				}
			}
			else
			{
				bRet = EnterZone(pPlayer);
				req(GetTickCount(), ENTER, job.handle, job.toZone, job.fromZone, true, bRet);
			}
		}
			break;
		case LEAVE:
		{
			bRet = LeaveZone(pPlayer);

			// 성공 / 실패 와 상관 없이 Stable 상태로 전환
			// LEAVE 를 처리 한다는건 Enter(toZone) -> 성공 -> Leave(fromZone) 인 경우 이다
			req(GetTickCount(), STABLE, job.handle, job.toZone, job.fromZone, false, false);
		}
			break;
		default:
			continue;
		}

		g_ZoneManager.ReqJob(req, job.fromZone);
	}

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

bool CZone::TryEnterZone()
{
	return m_Cnt.load() < m_MaxZoneManagerCount;
}


