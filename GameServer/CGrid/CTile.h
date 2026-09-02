#pragma once

#include "../CEntity.h"
#include "../CUtill/CEntityManagmentVector.h"
#include "../CUtill/CLockQueueh.h"
#include "../CUtill/CPacket.h"
#include <vector>
#include <cstdint>
#include <unordered_map>

class CMainWorld;

struct st_TileJob
{
	int type;
	CEntity* pEntity;
	uint64_t entityGeneration;
	uint64_t moveRevision;
};

struct st_TileBroadCast
{
	CEntity* pEntity;
	CPacket packet;
	uint64_t recipientGeneration;
};

class CTile
{
public:
	CTile() : m_iActive(0), m_iManagementID(-1), m_iEntityGeneration(0) {};
	~CTile() {};

private:
	CMainWorld* m_parent;

	std::atomic<int> m_iActive;
	int m_iManagementID;
	COORDINATE m_Coord;
	int m_iTileSize;

	st_Vector3F m_StartPos;
	st_Vector3F m_EndPos;

	CLQueue<st_TileJob> m_queue;
	CLQueue<st_TileBroadCast> m_queueBroadCast;
	std::vector<st_TileJob> m_vecJobBuffer;
	std::vector<st_TileBroadCast> m_vecBroadCastBuffer;

	CEntityVector m_vecPlayer;
	std::atomic<uint64_t> m_iEntityGeneration;
	std::unordered_map<CEntity*, uint64_t> m_mapEntityGeneration;

private:
	void TileJobRun();
	void TileBroadCast();
	bool IsVisibleAtGeneration(CEntity* pEntity, uint64_t generation) const;

	void NotifyEntityTileEnterAOI(CEntity* pEntity, uint64_t entityGeneration, uint64_t moveRevision);	// 나에게 타일 정보 생성 메시지
	void NotifyEntityTileLeaveAOI(CEntity* pEntity, uint64_t entityGeneration);	// 나에게 타일 정보 삭제 메시지(필요한가 에 대해서 클라에서 따로 타일 관리를 하면 안되는 것인가? 생각 해보기)
	void NotifyEntityTileEnterObj(CEntity* pEntity, uint64_t entityGeneration);	// 주위에 나 생성 메시지
	void NotifyEntityTileLeaveObj(CEntity* pEntity, uint64_t entityGeneration);	// 주위에 나 삭제 메시지

	void Broadcast(CPacket* pPacket, CEntity* pEntity, uint64_t recipientGeneration);

public:
	void Init(CMainWorld* parent, COORDINATE coord);

	void EnqueueJob(int type, CEntity* pEntity);
	void EnqueueBroadCast(CEntity* pEntity, CPacket* Packet);

	void Enqueue(int type, CEntity* pEntity);
	bool AddPlayer(CEntity* pEntity);
	bool RemovePlayer(CEntity* pEntity);

	void OnReigsterGrid(int Id) { m_iManagementID = Id; }

	int GetActiveCount() { return m_iActive.load(); }
	int GetManagementGrid() { return m_iManagementID; }
	COORDINATE& GetCoord() { return m_Coord; }

	void Update();
};
