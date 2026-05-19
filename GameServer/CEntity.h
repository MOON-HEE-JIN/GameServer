#pragma once
#include "Stub/StructDef.h"
#include "GameServerDef.h"
#include <atomic>

class CEntity
{
public:
	CEntity() {}
	~CEntity() {}
protected:
	int m_nEntityType = 0;

	int m_iChannel;
	std::atomic<int> m_OwnerZone;						// 처리 Zone 에 대한 id
	eZONESTATUS m_eZoneStatus;							// 현재 Zone 에 서 의 상태

	int m_iZoneVectorIndex = -1;
	int m_iMoveIndex = -1;

	float m_fMoveSpeed = 5.0f;
	COORDINATE m_stGridPos;
	st_Vector3F m_stPosition;
	st_Vector3F m_stGoalPosition;
	st_Vector3F m_stDirVector;
	eMOVESTATE m_eMoveState;
	//CZoneBase* m_pZone;
protected:
	void Reset();

public:
	bool MoveUpdate();

public:
	int GetChannel() { return m_iChannel; }
	int GetZoneID() { return m_OwnerZone.load(); }
	virtual int GetID() = 0;
	eZONESTATUS GetZoneStatus() { return m_eZoneStatus; }
	int GetZoneVectorIndex() { return m_iZoneVectorIndex; }
	int GetMoveIndex() { return m_iMoveIndex; }
	const COORDINATE& GetGridPos() { return m_stGridPos; }
	st_Vector3F GetPosition() { return m_stPosition; }
	st_Vector3F GetGoalPosition() { return m_stGoalPosition; }
	st_Vector3F GetDirVector() { return m_stDirVector; }
	float GetMoveSpeed() { return m_fMoveSpeed; }

	void SetZoneID(int channel, int zone) { m_iChannel = channel;  m_OwnerZone.store(zone); };
	void SetZoneStatus(eZONESTATUS type) { m_eZoneStatus = type; }
	void SetZoneVectorIndex(int index) { m_iZoneVectorIndex = index; }
	void SetMoveIndex(int index) { m_iMoveIndex = index; }
	void SetGridPos(COORDINATE& coord) { m_stGridPos = coord; }
	int MoveStart(st_Vector3F goal, st_Vector3F dir);
	void MoveComplete();
	int MoveStop(st_Vector3F pos);
};