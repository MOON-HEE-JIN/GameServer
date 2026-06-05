#include "CZoneBasic.h"

#include "../NetWork/CNetServer.h"
#include "../ZoneManager/CZoneManager.h"
#include "../Stub/EnumDef.h"
#include "../Log/CLog.h"

CZoneBasic::CZoneBasic(int channel, int ZoneID, int ProcID, int Maximum)
	: CZoneBase(channel, ZoneID, ProcID, Maximum)
{
}

CZoneBasic::~CZoneBasic()
{
}

void CZoneBasic::Init(int type, int width, int height)
{
	m_iWidth = width;
	m_iHeight = height;
}

void CZoneBasic::ChangeZoneProcess()
{
	ZONE_CHANGE_JOB job;

	while (m_queue.TryDequeue(job))
	{
		m_vecChangeZoneJobDebug.push_back(job);
		CPlayer* pPlayer = g_Net.GetPlayer(job.handle);
		if (pPlayer == nullptr)
			continue;

		// Player 가 종료 중 이라면 관련 없는 패킷 전부 Drop
		if (pPlayer->GetRelease() && job.type != eZONESTATUS::RELEASE)
		{
			continue;
		}

		ZONE_CHANGE_JOB req = job;
		req.time = GetTickCount();

		switch (job.type)
		{
		case NONE:
		case STABLE:
			break;
		case LOGIN:
		{
			EnterZone(pPlayer);

			// 새로운 Zone 에 입장 완료
			pPlayer->SetZoneStatus(eZONESTATUS::STABLE);
			{
				st_STC_ChangeZone pack;
				pack.ret = 0;
				pack.channel = job.toID;
				pack.zone = job.toZone;

				pPlayer->SendPacket(pack);
			}
		}
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
						pack.channel = job.toID;
						pack.zone = job.toZone;

						pPlayer->SendPacket(pack);
					}
				}
			}
			// From --> To Zone 으로 Enter 요청
			else
			{
				bool bRet = TryPush(pPlayer);
				// From Zone 에서 응답 보내기
				req.ack = true;
				req.ret = bRet;
				EnqueueChangeJob(job.fromID, job.fromZone, req);
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
					pack.channel = job.toID;
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
				EnqueueChangeJob(job.toID, job.toZone, req);
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
				if (bRet)
				{
					//CNetServer::DecrementProcCount(m_ID);
					SubCount();
				}
				else
				{
					g_LogServer.ELog("Error Release Zone");
				}
			}
			else
			{
				if (LeaveZone(pPlayer))
				{
					// OwnerZone 에서 Player Free 처리
					//CNetServer::DecrementPlayerCount();
					g_Net.FreePlayer(pPlayer);
				}
				else
				{
					// 해당 Zone 에서 관리하지 않음
					req.ack = true;
					EnqueueChangeJob(job.fromID, pPlayer->GetZoneID(), req);
				}
			}
		}
		break;
		default:
			break;
		}
	}
}

void CZoneBasic::Process()
{
}

bool CZoneBasic::EnterZone(CPlayer* pPlayer)
{
	pPlayer->SetZoneID(GetChannel(), GetZoneID());
	TryChangeZone(pPlayer->GetSessionHandle(), GetProcID());

	pPlayer->SetZoneVectorIndex(static_cast<int>(m_vecPlayers.size()));
	m_vecPlayers.push_back(pPlayer);

	pPlayer->SetZone(this);

	pPlayer->AddRef();
	OnEnterZone(pPlayer);
	return true;
}

bool CZoneBasic::LeaveZone(CPlayer* pPlayer)
{
	if (pPlayer == nullptr || m_vecPlayers.empty())
		return false;

	// 마지막 Player Index
	const int leaveIndex = pPlayer->GetZoneVectorIndex();
	if (leaveIndex < 0 || leaveIndex >= static_cast<int>(m_vecPlayers.size()))
		return false;

	pPlayer->ReleaseRef();
	// 마지막 플레이어 가져오기
	CPlayer* ePlayer = m_vecPlayers.back();

	// 마지막 플레이어 와 같지 않다면 교체
	if (ePlayer != pPlayer)
	{
		// 교체
		m_vecPlayers[leaveIndex] = ePlayer;
		ePlayer->SetZoneVectorIndex(leaveIndex);
	}

	m_vecPlayers.pop_back();

	
	// 존 떠날때 이벤트
	OnLeaveZone(pPlayer);
	return true;
}

bool CZoneBasic::Enqueue(ZONE_CHANGE_JOB& job)
{
	if (!m_bActive.load())
		return false;

	m_queue.Enqueue(std::move(job));
	return true;
}

bool CZoneBasic::TryPush(CPlayer* pPlayer)
{
	if (pPlayer == nullptr)
	{
		g_LogGame.ELog("ERROR pPlayer == nullptr CZoneBase::TryPush");
		return false;
	}

	if (GetCurCnt() + 1 >= GetMaximum())
	{
		g_LogGame.ELog("ERROR MaximumUser %d == m_iCurCnt %d  CZoneBase::TryPush", GetMaximum(), GetCurCnt());
		return false;
	}

	AddCount();
	return true;
}

bool CZoneBasic::TryEnterZone()
{
	return GetCurCnt() < GetMaximum();
}

void CZoneBasic::SendZoneCast(CPacket* pPacket, CPlayer* pPlayer)
{
	int nLoop = static_cast<int>(m_vecPlayers.size());
	for (int i = 0; i < nLoop; i++)
	{
		if (m_vecPlayers[i] == pPlayer)
			continue;

		m_vecPlayers[i]->SendPacket(pPacket);
	}
}
