#include "CEntity.h"
#include "ZoneManager/CZoneManager.h"
#include "./CUtill/CUtill.h"
#include "Stub/StructDef.h"

bool CEntity::MoveUpdate()
{
    float speedDx = m_stDirVector.X * m_fMoveSpeed * FIXED_DELTA;
	float speedDy = m_stDirVector.Y * m_fMoveSpeed * FIXED_DELTA;
    float speedDz = m_stDirVector.Z * m_fMoveSpeed * FIXED_DELTA;

    float speeddist = m_fMoveSpeed * FIXED_DELTA * m_fMoveSpeed * FIXED_DELTA;
    float remaindist = m_stPosition.DistanceToNSquared(m_stGoalPosition);
	
    if (remaindist <= speeddist)
    {
        m_stPosition = m_stGoalPosition;
		MoveComplete();
        return true;
    }
    else
    {
		m_stPosition += st_Vector3F(speedDx, speedDy, speedDz);
    }

#ifdef __DEBUG__

#endif // __DEBUG__

    return false;
}

int CEntity::MoveStart(st_Vector3F goal, st_Vector3F dir)
{
    // 움직일수 있는 상태인지 체크
    if (0)
    {

    }

    m_eMoveState = eMOVESTATE::MOVEING;
    m_stGoalPosition = goal;
    m_stDirVector = dir;
    
    g_ZoneManager.PushZoneMoveVector(this);

    return 0;
}

void CEntity::MoveComplete()
{
    st_STC_MoveStop res;
    res.pos = m_stPosition;
    res.type = m_nEntityType;
    res.ret = 0;
    
    switch (m_nEntityType)
    {
    case 0:
        res.ID = ((CPlayer*)this)->GetPlayerHandle();
        break;
    default:
        break;
    }
    CPacket pack;
    pack << res;

	// Zone Broadcast
    g_ZoneManager.SendZone(GetZoneID(), &pack);
}

int CEntity::MoveStop(st_Vector3F pos)
{
    // 움직일수 없는 상태인지 체크
    if (0)
    {

    }

    m_eMoveState = eMOVESTATE::STOPPED;

    g_ZoneManager.PopZoneMoveVector(this);

    return 0;
}
