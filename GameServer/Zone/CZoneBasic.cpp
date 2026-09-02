#include "CZoneBasic.h"

#include "../NetWork/CNetServer.h"
#include "../ZoneManager/CZoneManager.h"
#include "../Stub/EnumDef.h"
#include "../Log/CLog.h"

namespace
{
	void SendZoneChangeResult(CPlayer* pPlayer, int ret, int channel, int zone, const st_Vector3F& spawn)
	{
		st_STC_ChangeZone packet{};
		packet.ret = ret;
		packet.channel = channel;
		packet.zone = zone;
		packet.spawn = spawn;
		pPlayer->SendPacket(packet);
	}

	class CZoneJobRef
	{
	public:
		explicit CZoneJobRef(CPlayer* pPlayer) : m_pPlayer(pPlayer) {}
		~CZoneJobRef() { m_pPlayer->ReleaseMagRef(); }

	private:
		CPlayer* m_pPlayer;
	};
}

CZoneBasic::CZoneBasic(int channel, int ZoneID, int ProcID, int Maximum)
	: CZoneBase(channel, ZoneID, ProcID, Maximum)
{
	m_stSpawnPos.Zero();
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
		CPlayer* pPlayer = job.pPlayer;
		if (pPlayer == nullptr)
			continue;
		CZoneJobRef jobRef(pPlayer);

		// Player가 종료 중이면 전환 예약을 되돌린 뒤 관련 작업을 버린다.
		if (pPlayer->GetRelease() && job.type != eZONESTATUS::RELEASE)
		{
			if (job.type == eZONESTATUS::LOGIN)
			{
				RollbackPush();
			}
			else if ((job.type == eZONESTATUS::ENTER && job.ack) ||
				job.type == eZONESTATUS::LEAVE)
			{
				CZoneBasic* pToZone = g_ZoneManager.GetZone(job.toID, job.toZone);
				if (pToZone != nullptr)
					pToZone->RollbackPush();
			}
			continue;
		}

		ZONE_CHANGE_JOB req = job;
		
		switch (job.type)
		{
		case NONE:
		case STABLE:
			break;
		case LOGIN:
		{
			if (!EnterZone(pPlayer))
			{
				RollbackPush();
				g_Net.PlayerDisConnect(pPlayer->GetSessionHandle());
				break;
			}

			pPlayer->SetZoneStatus(eZONESTATUS::STABLE);
			SendZoneChangeResult(pPlayer, ERROR_CODE::NOT_ERROR,
				job.toID, job.toZone, GetSpawnPos());
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
					if (!Enqueue(req))
					{
						CZoneBasic* pToZone = g_ZoneManager.GetZone(job.toID, job.toZone);
						if (pToZone != nullptr)
							pToZone->RollbackPush();
						pPlayer->SetZoneStatus(eZONESTATUS::STABLE);
						SendZoneChangeResult(pPlayer, ERROR_CODE::NOT_FIND_PID,
							job.toID, job.toZone, st_Vector3F{});
					}
				}
				else
				{
					// Zone Enter 실패
					pPlayer->SetZoneStatus(eZONESTATUS::STABLE);
					SendZoneChangeResult(pPlayer, ERROR_CODE::NOT_FIND_PID,
						job.toID, job.toZone, st_Vector3F{});
				}
			}
			// From --> To Zone 으로 Enter 요청
			else
			{
				bool bRet = TryPush(pPlayer);
				// From Zone 에서 응답 보내기
				req.ack = true;
				req.ret = bRet;
				if (!EnqueueChangeJob(job.fromID, job.fromZone, req))
				{
					if (bRet)
						RollbackPush();
					pPlayer->SetZoneStatus(eZONESTATUS::STABLE);
					SendZoneChangeResult(pPlayer, ERROR_CODE::NOT_FIND_PID,
						job.toID, job.toZone, st_Vector3F{});
				}
			}
		}
		break;
		case LEAVE:
		{
			// From Zone 에서 Leave 완료 To Zone 에 넣어주기
			if (job.ack)
			{
				if (!job.ret)
				{
					RollbackPush();
					pPlayer->SetZoneStatus(eZONESTATUS::STABLE);
					SendZoneChangeResult(pPlayer, ERROR_CODE::NOT_FIND_PID,
						job.toID, job.toZone, st_Vector3F{});
					break;
				}

				if (!EnterZone(pPlayer))
				{
					RollbackPush();
					g_LogServer.ELog("Zone enter failed after source leave. Player:%d To:%d/%d",
						pPlayer->GetID(), job.toID, job.toZone);
					g_Net.PlayerDisConnect(pPlayer->GetSessionHandle());
					break;
				}

				// MainWorld는 Grid/Tile 등록이 끝난 뒤 성공 응답과 STABLE 상태를 공개한다.
				if (!GetMainWorld())
				{
					pPlayer->SetZoneStatus(eZONESTATUS::STABLE);
					SendZoneChangeResult(pPlayer, ERROR_CODE::NOT_ERROR,
						job.toID, job.toZone, GetSpawnPos());
				}
			}
			else
			{
				bool bRet = LeaveZone(pPlayer);
				req.ack = true;
				req.ret = bRet;
				// To Zone 에게 Leave 완료를 보낸다
				if (!EnqueueChangeJob(job.toID, job.toZone, req))
				{
					CZoneBasic* pToZone = g_ZoneManager.GetZone(job.toID, job.toZone);
					if (pToZone != nullptr)
						pToZone->RollbackPush();

					bool restored = !bRet;
					if (bRet && TryPush(pPlayer))
					{
						restored = EnterZone(pPlayer);
						if (!restored)
							RollbackPush();
					}

					if (!restored)
					{
						g_LogServer.ELog("Zone restore failed after ack enqueue failure. Player:%d From:%d/%d",
							pPlayer->GetID(), job.fromID, job.fromZone);
						g_Net.PlayerDisConnect(pPlayer->GetSessionHandle());
						break;
					}
					pPlayer->SetZoneStatus(eZONESTATUS::STABLE);
					SendZoneChangeResult(pPlayer, ERROR_CODE::NOT_FIND_PID,
						job.toID, job.toZone, st_Vector3F{});
				}
			}
		}
		break;
		case RELEASE:
		{
			// ack == true 이전 Zone 관리가 아니라서 다시 보내는것
			if (job.ack)
			{
				if (LeaveZone(pPlayer))
				{
					//CNetServer::DecrementProcCount(m_ID);
					g_Net.FreePlayer(pPlayer);
				}
				else
				{
					g_Net.FreePlayer(pPlayer);
					g_LogServer.ELog("Error Release Zone");
				}
			}
			else
			{
				if (!LeaveZone(pPlayer))
				{
					// 해당 Zone 에서 관리하지 않음
					req.ack = true;
					if (!EnqueueChangeJob(job.fromID, pPlayer->GetZoneID(), req))
						g_Net.FreePlayer(pPlayer);
				}
				else
					g_Net.FreePlayer(pPlayer);
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
	if (pPlayer == nullptr)
		return false;

	if (!m_vecEntitys.AddEntity(pPlayer))
		return false;

	if (!TryChangeZone(pPlayer->GetSessionHandle(), GetProcID()))
	{
		m_vecEntitys.RemoveEntity(pPlayer);
		return false;
	}

	pPlayer->SetZoneID(GetChannel(), GetZoneID());
	pPlayer->SetZone(this);

	OnEnterZone(pPlayer);
	return true;
}

bool CZoneBasic::LeaveZone(CPlayer* pPlayer)
{
	if (pPlayer == nullptr)
		return false;

	if (!m_vecEntitys.RemoveEntity(pPlayer))	
		return false;

	// MainWorld movement containers belong to Grid workers and are removed by the leave job.
	if (!GetMainWorld())
		pPlayer->MoveStop(pPlayer->GetPosition());
	
	// 존 떠날때 이벤트
	OnLeaveZone(pPlayer);
	if (!SubCount())
		g_LogServer.ELog("Zone count underflow. Channel:%d Zone:%d", GetChannel(), GetZoneID());
	return true;
}

bool CZoneBasic::Enqueue(ZONE_CHANGE_JOB& job)
{
	if (!m_bActive.load() || job.pPlayer == nullptr)
		return false;

	job.pPlayer->AddMagRef();
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

	if (!TryAddCount())
	{
		g_LogGame.ELog("ERROR MaximumUser %d == m_iCurCnt %d  CZoneBase::TryPush", GetMaximum(), GetCurCnt());
		return false;
	}
	return true;
}

bool CZoneBasic::TryEnterZone()
{
	return GetCurCnt() < GetMaximum();
}

void CZoneBasic::BoradCast(CPacket* pPacket, COORDINATE pivot, CPlayer* pPlayer)
{
	int nLoop = m_vecEntitys.GetSize();
	const std::vector<CEntity*>& m_vecPlayers = m_vecEntitys.GetVector();
	for (int i = 0; i < nLoop; i++)
	{
		if (m_vecPlayers[i] == pPlayer)
			continue;

		CPlayer* pSendPlayer = dynamic_cast<CPlayer*>(m_vecPlayers[i]);
		if (pSendPlayer == nullptr)
			continue;

		pSendPlayer->SendPacket(pPacket);
	}
}
