#pragma once

#include "../CEntity.h"
#include "../CUtill/CEntityManagmentVector.h"
#include "../CUtill/CLockQueueh.h"
#include "../CUtill/CPacket.h"

class CMainWorld;

struct st_TileJob
{
	int type;
	CEntity* pEntity;
};

struct st_TileBroadCast
{
	CEntity* pEntity;
	CPacket packet;
};

class CTile
{
public:
	CTile() : m_iActive(0), m_iManagementID(-1) {};
	~CTile() {};

private:
	CMainWorld* m_parent;

	std::atomic<int> m_iActive;
	int m_iManagementID;
	COORDINATE m_Coord;
	st_Vector3F m_StartPos;
	st_Vector3F m_EndPos;

	CLQueue<st_TileJob> m_queue;
	CLQueue<st_TileBroadCast> m_queueBroadCast;

	CEntityVector m_vecPlayer;

private:
	void TileJobRun();
	void TileBroadCast();

	void NotifyEntityTileEnterAOI(CEntity* pEntity);	// 나에게 타일 정보 생성 메시지
	void NotifyEntityTileLeaveAOI(CEntity* pEntity);	// 나에게 타일 정보 삭제 메시지(필요한가 에 대해서 클라에서 따로 타일 관리를 하면 안되는 것인가? 생각 해보기)
	void Broadcast(CPacket* pPacket, CEntity* pEntity = nullptr);

public:
	void Init(CMainWorld* parent, COORDINATE coord, st_Vector3F start, st_Vector3F end);

	void EnqueueJob(int type, CEntity* pEntity);
	void EnqueueBroadCast(CEntity* pEntity, CPacket* Packet);

	bool AddPlayer(CEntity* pEntity);
	bool RemovePlayer(CEntity* pEntity);

	void OnReigsterGrid(int Id) { m_iManagementID = Id; }

	int GetActiveCount() { return m_iActive.load(); }
	int GetManagementGrid() { return m_iManagementID; }
	COORDINATE& GetCoord() { return m_Coord; }

	void Update();
};
