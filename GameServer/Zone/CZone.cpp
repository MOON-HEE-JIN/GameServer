#include "CZone.h"
#include "../NetWork/CNetServer.h"
#include "../ZoneManager/CZoneManager.h"
#include "../Stub/EnumDef.h"
#include "../Stub/PacketEnumDef.h"

CZone::CZone(int channel, int ZoneID, int ProcID, int Maximum)
	: CZoneBasic(channel, ZoneID, ProcID, Maximum)
{
}

CZone::~CZone()
{

}


bool CZone::PushMoveVector(CEntity* pEntity)
{
	if (pEntity->GetMoveVectorIndex() != -1)
		return false;

	int index = static_cast<int>(m_vecEntityMoveVector.size());
	m_vecEntityMoveVector.push_back(pEntity);
	pEntity->SetMoveVectorIndex(index);
	return true;
}

void CZone::PopMoveVector(CEntity* pEntity)
{
	int index = pEntity->GetMoveVectorIndex();
	
	if (index == -1)
		return;

	int lastindex = static_cast<int>(m_vecEntityMoveVector.size()) - 1;
	pEntity->SetMoveVectorIndex(-1);
	if (lastindex < 0)
		return;
	if (index != lastindex)
	{
		CEntity* pLast = m_vecEntityMoveVector[lastindex];
		m_vecEntityMoveVector[index] = pLast;
		pLast->SetMoveVectorIndex(index);
	}

	m_vecEntityMoveVector.pop_back();
	pEntity->SetMoveVectorIndex(-1);
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
	int nLoop = static_cast<int>(m_vecEntityMoveVector.size());
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
