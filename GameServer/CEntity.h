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
	
	// 실제 Grid 등록 상태와 전환 중 패킷 목적지를 분리한다.
	// 두 값은 Proxy/Grid Thread에서 동시에 조회/갱신되므로 atomic이어야 한다.
	std::atomic<int> m_iGridID;
	std::atomic<int> m_iPendingGridID;
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
	int GetGridID() const { return m_iGridID.load(std::memory_order_acquire); }
	int GetRoutingGridID() const
	{
		int PendingGridID = m_iPendingGridID.load(std::memory_order_acquire);
		return PendingGridID >= 0 ? PendingGridID : GetGridID();
	}
	const COORDINATE& GetTilePos() { return m_stTilePos; }
	st_Vector3F GetPosition() { return m_stPosition; }
	st_Vector3F GetGoalPosition() { return m_stGoalPosition; }
	st_Vector3F GetDirVector() { return m_stDirVector; }
	float GetMoveSpeed() { return m_fMoveSpeed; }
	int GetMoveState() { return m_eMoveState; }
	int GetType() { return m_nEntityType; }

	void SetZoneID(int channel, int zone) { m_iChannel = channel;  m_OwnerZone.store(zone); };
	void SetZoneStatus(eZONESTATUS type) { m_eZoneStatus = type; }
	void SetGridID(int id) { m_iGridID.store(id, std::memory_order_release); }
	void ClearGridID(int expectedID)
	{
		m_iGridID.compare_exchange_strong(expectedID, -1,
			std::memory_order_acq_rel, std::memory_order_acquire);
	}
	void BeginGridTransfer(int destinationGridID)
	{
		m_iPendingGridID.store(destinationGridID, std::memory_order_release);
	}
	void CompleteGridTransfer()
	{
		m_iPendingGridID.store(-1, std::memory_order_release);
	}
	void SetTilePos(COORDINATE& coord) { m_stTilePos = coord; }
	void SetPosition(st_Vector3F pos) { m_stPosition = pos; }
	int MoveStart(st_Vector3F goal, st_Vector3F dir);
	void MoveComplete();
	int MoveStop(st_Vector3F pos);
};
