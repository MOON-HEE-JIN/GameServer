#pragma once
#include "Stub/StructDef.h"
#include "GameServerDef.h"
#include <vector>
#include <atomic>
#include "CObject.h"
class CEntity : public CObject
{
public:
	CEntity();
	~CEntity() {}
protected:
	int m_nEntityType = 0;

	int m_iChannel;
	std::atomic<int> m_OwnerZone;						// 처리 Zone 에 대한 id
	eZONESTATUS m_eZoneStatus;							// 현재 Zone 에 서 의 상태

	std::vector<int> m_vecIndex;

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

public:
	int GetVectorIndex(int type);
	int GetZoneVectorIndex() { return m_vecIndex[EIndexType::VECTOR_INDEX_ZONE]; };
	int GetGridVectorIndex() { return m_vecIndex[EIndexType::VECTOR_INDEX_GRID]; };
	int GetTileVectorIndex() { return m_vecIndex[EIndexType::VECTOR_INDEX_TILE]; };
	int GetMoveVectorIndex() { return m_vecIndex[EIndexType::VECTOR_INDEX_MOVE]; };

	bool SetVectorIndex(int type, int value);
	void SetZoneVectorIndex(int value) { m_vecIndex[EIndexType::VECTOR_INDEX_ZONE] = value; };
	void SetGridVectorIndex(int value) { m_vecIndex[EIndexType::VECTOR_INDEX_GRID] = value; };
	void SetTileVectorIndex(int value) { m_vecIndex[EIndexType::VECTOR_INDEX_TILE] = value; };
	void SetMoveVectorIndex(int value) { m_vecIndex[EIndexType::VECTOR_INDEX_MOVE] = value; };
public:
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

	void SetZoneID(int channel, int zone) { m_iChannel = channel;  m_OwnerZone.store(zone); };
	void SetZoneStatus(eZONESTATUS type) { m_eZoneStatus = type; }
	void SetGridID(int id) { m_iGridID = id; }
	void SetTilePos(COORDINATE& coord) { m_stTilePos = coord; }
	void SetPosition(st_Vector3F pos) { m_stPosition = pos; }
	int MoveStart(st_Vector3F goal, st_Vector3F dir);
	void MoveComplete();
	int MoveStop(st_Vector3F pos);
};