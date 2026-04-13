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

bool CZone::PushTemp(CPlayer* pPlayer)
{
	if (pPlayer == nullptr)
		return false;

	if (m_Cnt.load() + 1 >= m_MaxZoneManagerCount)
		return false;

	// 이미 존재 하는 Player 라면 false
	if (m_mapIDtoIndex.find(pPlayer->GetPlayerHandle()) != m_mapIDtoIndex.end())
		return false;

	if (!TryChangeZone(pPlayer->GetSessionHandle(), m_ID))
		return false;

	m_Cnt.fetch_add(1);
	//CNetServer::IncrementProcCount(m_ZonePid);
	return true;
}

void CZone::PushMoveVector(CEntity* pEntity)
{
	if (pEntity->GetMoveIndex() != -1)
		return;
	
	int index = m_vecEntityMoveVector.size();
	m_vecEntityMoveVector.push_back(pEntity);
	pEntity->SetMoveIndex(index);
}

void CZone::PopMoveVector(CEntity* pEntity)
{
	int index = pEntity->GetMoveIndex();
	
	if (index == -1)
		return;

	int lastindex = m_vecEntityMoveVector.size() - 1;
	pEntity->SetMoveIndex(-1);
	if (lastindex < 0)
		return;
	if (index != lastindex)
	{
		CEntity* pLast = m_vecEntityMoveVector[lastindex];
		m_vecEntityMoveVector[index] = pLast;
		pLast->SetMoveIndex(index);
	}

	m_vecEntityMoveVector.pop_back();
	pEntity->SetMoveIndex(-1);
}

bool CZone::SendZoneInfo(CPlayer* pPlayer)
{
	if (pPlayer->GetZoneID() != m_ID)
		return false;

	int nLoop = m_vecPlayer.size();
	int index = 0;

	st_STC_EnterZone info = { 0, };

	while (1)
	{
		int i = index++;
		if (index >= nLoop)
			break;
		if (m_vecPlayer[i] == pPlayer || m_vecPlayer[i] == nullptr)
			continue;

		info.info[info.Loop1].type = 0;
		info.info[info.Loop1].ID = pPlayer->GetPlayerHandle();
		info.info[info.Loop1].pos = pPlayer->GetPosition();
		info.Loop1++;
		if (info.Loop1 >= 50)
		{
			pPlayer->SendPacket(info);
			ZeroMemory(&info, sizeof(info));
		}
	}
	if (info.Loop1 > 0)
	{
		pPlayer->SendPacket(info);
	}
	return true;
}

void CZone::SendBroadCast(CPacket* pPacket, CPlayer* pPlayer)
{
	int nLoop = m_vecPlayer.size();
	for (int i = 0; i < nLoop; i++)
	{
		if (m_vecPlayer[i] == pPlayer)
			continue;

		m_vecPlayer[i]->SendPacket(pPacket);
	}
}

void CZone::ZoneMoveJobProcess()
{
	ZONE_JOB job;

	while (m_queue.TryDequeue(job))
	{
		CPlayer* pPlayer = g_Net.GetPlayer(job.handle);
		if (pPlayer == nullptr)
			continue;

		// Player 가 종료 중 이라면 관련 없는 패킷 전부 Drop
		if (pPlayer->GetRelease())
			if (job.type != eZONESTATUS::RELEASE)
				continue;

		ZONE_JOB req = job;
		req.time = GetTickCount();

		switch (job.type)
		{
		case NONE:
			break;
		case STABLE:
			break;
		case ENTER:
		{
			// To --> From Zone 으로 Enter 응답 
			if (job.ack)
			{
				if (job.ret)
				{
					// Zone Enter 성공
					pPlayer->SetZoneStatus(eZONESTATUS::ENTER); // Leave -> Enter 변경
					
					// 자신 Zone 에게 Leave 요청 보내기
					// 여기서 Leave 를 처리해도 되지만 규칙성을 위해서 넣어준다
					req.type = eZONESTATUS::LEAVE;
					req.ack = false;
					m_queue.Enqueue(req);
				}
				else
				{
					// Zone Enter 실패
					pPlayer->SetZoneStatus(eZONESTATUS::STABLE);
					{
						st_STC_ChangeZone pack;
						pack.ret = ERROR_CODE::NOT_FIND_PID;
						pack.zone = job.toZone;

						pPlayer->SendPacket(pack);
					}
				}
			}
			// From --> To Zone 으로 Enter 요청
			else
			{
				bool bRet = PushTemp(pPlayer);
				// From Zone 에서 응답 보내기
				req.ack = true;
				req.ret = bRet;
				g_ZoneManager.ReqJob(req, job.fromZone);
			}
		}
			break;
		case LEAVE:
		{
			// From Zone 에서 Leave 완료 To Zone 에 넣어주기
			if (job.ack)
			{
				EnterZone(pPlayer);
				
				// 새로운 Zone 에 입장 완료
				pPlayer->SetZoneStatus(eZONESTATUS::STABLE);
				{
					st_STC_ChangeZone pack;
					pack.ret = 0;
					pack.zone = job.toZone;

					pPlayer->SendPacket(pack);
				}
			}
			else
			{
				bool bRet = LeaveZone(pPlayer);
				req.ack = true;
				req.ret = bRet;
				// To Zone 에게 Leave 완료를 보낸다
				g_ZoneManager.ReqJob(req, job.toZone);
			}
		}
			break;
		case RELEASE:
		{
			// ack == true 이전 Zone 관리가 아니라서 다시 보내는것
			if (job.ack)
			{
				bool bRet = LeaveZone(pPlayer);
				
				// OwnerZone 에서 Player Free 처리 || 아무 Zone 에서 관리하지 않음
				//CNetServer::DecrementPlayerCount();
				g_Net.FreePlayer(pPlayer);

				// ToZone 에서 관리 vector 전에 Release 되었을때
				if (!bRet)
				{
					//CNetServer::DecrementProcCount(m_ID);
					m_Cnt.fetch_sub(1);
				}
			}
			else
			{
				if (pPlayer->GetZoneID() == m_ID)
				{
					bool bRet = LeaveZone(pPlayer);
					
					// OwnerZone 에서 Player Free 처리
					//CNetServer::DecrementPlayerCount();
					g_Net.FreePlayer(pPlayer);
				}
				else
				{
					// 해당 Zone 에서 관리하지 않음
					req.ack = true;
					g_ZoneManager.ReqJob(req, pPlayer->GetZoneID());
				}
			}
		}
			break;
		default:
			break;
		}
	}

}

void CZone::ZoneEntityMoveProcess()
{
	int nLoop = m_vecEntityMoveVector.size();
	std::vector<CEntity*> vec;
	int eraseCnt = 0;
	for (int i = 0; i < nLoop; i++)
	{
		if (m_vecEntityMoveVector[i]->MoveUpdate())
		{
			// 이동이 완료된 CEntity;
			vec.push_back(m_vecEntityMoveVector[i]);
			eraseCnt++;
		}
	}

	for (int i = 0; i < eraseCnt; i++)
	{
		PopMoveVector(vec[i]);
	}
}

bool CZone::EnterZone(CPlayer* pPlayer)
{
	pPlayer->SetZoneID(m_ID);

	m_mapIDtoIndex[pPlayer->GetPlayerHandle()] = static_cast<int>(m_vecPlayer.size());
	m_vecPlayer.push_back(pPlayer);

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

	// 이동 중이라면 Vector 에서 제거
	PopMoveVector(pPlayer);

	m_vecPlayer.pop_back();
	//m_mapIDtoIndex.erase(iter);
	//CNetServer::DecrementProcCount(m_ZonePid);
	m_Cnt.fetch_sub(1);



	return true;
}

bool CZone::TryEnterZone()
{
	return m_Cnt.load() <= m_MaxZoneManagerCount;
}


