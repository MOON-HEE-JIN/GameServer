#pragma once
#include "Stub/StructDef.h"
#include "GameServerDef.h"
#include <vector>
#include <atomic>

class CEntity
{
public:
	CEntity();
	~CEntity() {}
private:
	/*
	* m_iRef 사용처
	* - OnClientJoin 에서 1 로 시작 FreePlayer 에서 -1
	* - ReqEnterLoginZone +1 이유 순서 Leave -> Enter +1 을 하지 않으면 바로 종료
	* - EnterZone 에서 +1, LeaveZone 에서 -1
	* - Grid AddPlayer +1, RemovePlayer -1
	*/
	std::atomic<int> m_iRef;
	std::atomic<int> m_iMagRef;
	std::atomic<int> m_iQueRef;
protected:
	int m_nEntityType = 0;

	int m_iChannel;
	std::atomic<int> m_OwnerZone;						// 처리 Zone 에 대한 id
	eZONESTATUS m_eZoneStatus;							// 현재 Zone 에 서 의 상태

	float m_fMoveSpeed = 5.0f;
	
	int m_iGridID;
	COORDINATE m_stTilePos;

	st_Vector3F m_stPosition;
	st_Vector3F m_stGoalPosition;
	st_Vector3F m_stDirVector;
	eMOVESTATE m_eMoveState;
	//CZoneBase* m_pZone;
protected:
	void Reset();

public:
	bool MoveUpdate();

protected:
	int GetRef() { return m_iRef.load(); }
	void AddRef() { m_iRef.fetch_add(1); }
	void ReleaseRef();
	virtual void OnRelease() { delete this; };
public:
	void AddMagRef();
	void ReleaseMagRef();
	void AddQueRef();
	void ReleaseQueRef();
public:
	int GetEntityType() { return m_nEntityType; }
	int GetChannel() { return m_iChannel; }
	int GetZoneID() { return m_OwnerZone.load(); }
	virtual int GetID() = 0;
	eZONESTATUS GetZoneStatus() { return m_eZoneStatus; }
	const int& GetGridID() { return m_iGridID; }
	const COORDINATE& GetTilePos() { return m_stTilePos; }
	st_Vector3F GetPosition() { return m_stPosition; }
	st_Vector3F GetGoalPosition() { return m_stGoalPosition; }
	st_Vector3F GetDirVector() { return m_stDirVector; }
	float GetMoveSpeed() { return m_fMoveSpeed; }
	int GetMoveState() { return m_eMoveState; }
	int GetType() { return m_nEntityType; }

	void SetZoneID(int channel, int zone) { m_iChannel = channel;  m_OwnerZone.store(zone); };
	void SetZoneStatus(eZONESTATUS type) { m_eZoneStatus = type; }
	void SetGridID(int id) { m_iGridID = id; }
	void SetTilePos(COORDINATE& coord) { m_stTilePos = coord; }
	void SetPosition(st_Vector3F pos) { m_stPosition = pos; }
	int MoveStart(st_Vector3F goal, st_Vector3F dir);
	void MoveComplete();
	int MoveStop(st_Vector3F pos);
};