#include "CEntity.h"
#include "ZoneManager/CZoneManager.h"
#include "./CUtill/CUtill.h"
#include "Stub/StructDef.h"

CEntity::CEntity()
{
    m_vecIndex.resize(EIndexType::VECTOR_INDEX_END);
}

void CEntity::Reset()
{
    m_nEntityType = 0;

    m_iChannel = 0;
    m_OwnerZone.store(0);			                			// 처리 Zone 에 대한 id
    m_eZoneStatus = eZONESTATUS::NONE;							// 현재 Zone 에 서 의 상태

    for (int i = 0; i < EIndexType::VECTOR_INDEX_END; i++)
    {
        m_vecIndex[i] = -1;
    }
    m_fMoveSpeed = 5.0f;
    m_stPosition.Zero();
    m_stGoalPosition.Zero();
    m_stDirVector.Zero();
    m_eMoveState = eMOVESTATE::STOPPED;
}

bool CEntity::MoveUpdate()
{
    float speed = m_fMoveSpeed * FIXED_DELTA;

    m_stDirVector = m_stPosition.Direction(m_stGoalPosition);

    float speedDx = m_stDirVector.X * speed;
	float speedDy = m_stDirVector.Y * speed;
    float speedDz = m_stDirVector.Z * speed;

    float speeddist = speed * speed;
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

    if (m_stDirVector.X > 0)
    {
        if (m_stPosition.X > m_stGoalPosition.X)
        {
            int a = 100;
            a++;
        }
    }
    else
    {
        if (m_stPosition.X < m_stGoalPosition.X)
        {
            int a = 100;
            a++;
        }
    }
    if (m_stDirVector.Y > 0)
    {
        if (m_stPosition.Y > m_stGoalPosition.Y)
        {
            int a = 100;
            a++;
        }
    }
    else
    {
        if (m_stPosition.Y < m_stGoalPosition.Y)
        {
            int a = 100;
            a++;
        }
    }
    if (m_stDirVector.Z > 0)
    {
        if (m_stPosition.Z > m_stGoalPosition.Z)
        {
            int a = 100;
            a++;
        }
    }
    else
    {
        if (m_stPosition.Z < m_stGoalPosition.Z)
        {
            int a = 100;
            a++;
        }
    }

    return false;
}

int CEntity::GetVectorIndex(int type)
{
    if (type >= EIndexType::VECTOR_INDEX_END)
        return -1;
    return m_vecIndex[type];
}

bool CEntity::SetVectorIndex(int type, int value)
{
    if (type >= EIndexType::VECTOR_INDEX_END)
        return false;
    
    m_vecIndex[type] = value;
    return true;
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
        res.ID = ((CPlayer*)this)->GetID();
        break;
    default:
        break;
    }
    CPacket pack;
    pack << res;
    
	// Zone Broadcast
    g_ZoneManager.SendZone(GetChannel(), GetZoneID(), &pack);
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
