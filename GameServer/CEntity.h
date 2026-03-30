#pragma once
#include "Stub/StructDef.h"
#include "GameServerDef.h"
#include <atomic>

class CEntity
{
protected:
	int m_nEntityType = 0;

	std::atomic<int> m_OwnerZone;						// 처리 Zone 에 대한 id
	eZONESTATUS m_eZoneStatus;							// 현재 Zone 에 서 의 상태

	int m_nMoveIndex = -1;
	float m_fMoveSpeed = 5.0f;
	st_Vector3F m_stPosition;
	st_Vector3F m_stGoalPosition;
	st_Vector3F m_stDirVector;
	eMOVESTATE m_eMoveState;

public:
	bool MoveUpdate();

public:
	int GetZoneID() { return m_OwnerZone.load(); }
	eZONESTATUS GetZoneStatus() { return m_eZoneStatus; }
	int GetMoveIndex() { return m_nMoveIndex; }
	st_Vector3F GetPosition() { return m_stPosition; }
	st_Vector3F GetGoalPosition() { return m_stGoalPosition; }
	st_Vector3F GetDirVector() { return m_stDirVector; }
	float GetMoveSpeed() { return m_fMoveSpeed; }

	void SetZoneID(int zone) { m_OwnerZone.store(zone); };
	void SetZoneStatus(eZONESTATUS type) { m_eZoneStatus = type; }
	void SetMoveIndex(int index) { m_nMoveIndex = index; }
	int MoveStart(st_Vector3F goal, st_Vector3F dir);
	void MoveComplete();
	int MoveStop(st_Vector3F pos);
};