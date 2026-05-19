#include "CZone.h"
#include "../NetWork/CNetServer.h"
#include "../ZoneManager/CZoneManager.h"
#include "../Stub/EnumDef.h"
#include "../Stub/PacketEnumDef.h"

CZone::CZone(int channel, int ZoneID, int ProcID, int Maximum)
	: CZoneBase(channel, ZoneID, ProcID, Maximum)
{
}

CZone::~CZone()
{

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
	if (pPlayer->GetZoneID() != GetZoneID())
		return false;

	int nLoop = m_vecPlayers.size();
	int index = 0;

	st_STC_EnterZone info = { 0, };
	
	while (1)
	{
		int i = index++;
		if (index >= nLoop)
			break;
		if (m_vecPlayers[i] == pPlayer || m_vecPlayers[i] == nullptr)
			continue;

		info.info[info.Loop1].type = 0;
		info.info[info.Loop1].ID = pPlayer->GetID();
		info.info[info.Loop1].pos = pPlayer->GetPosition();
		info.Loop1++;
		if (info.Loop1 >= 48)
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

void CZone::OnLeaveZone(CPlayer* pPlayer)
{
	PopMoveVector(pPlayer);
}

void CZone::Process()
{
	ZoneEntityMoveProcess();
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
