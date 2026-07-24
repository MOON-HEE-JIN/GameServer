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


void CZone::PushMoveVector(CEntity* pEntity)
{
	m_vecEntityMoveVector.AddEntity(pEntity);
}

void CZone::PopMoveVector(CEntity* pEntity)
{
	m_vecEntityMoveVector.RemoveEntity(pEntity);
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
	int nLoop = m_vecEntityMoveVector.GetCount();
	const std::vector<CEntity*>& vec = m_vecEntityMoveVector.GetVector();
	std::vector<CEntity*> vecCompleteMove;
	int eraseCnt = 0;
	for (int i = 0; i < nLoop; i++)
	{
		if (vec[i]->MoveUpdate())
		{
			// 이동이 완료된 CEntity;
			vecCompleteMove.push_back(vec[i]);
			eraseCnt++;
		}
	}

	for (int i = 0; i < eraseCnt; i++)
	{
		PopMoveVector(vecCompleteMove[i]);
	}
}
