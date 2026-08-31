#pragma once

#include "../CEntity.h"
#include "../CUtill/CEntityManagmentVector.h"
#include "../CUtill/CLockQueueh.h"
#include "../CUtill/CPacket.h"
#include <vector>

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
	int m_iTileSize;

	st_Vector3F m_StartPos;
	st_Vector3F m_EndPos;

	CLQueue<st_TileJob> m_queue;
	CLQueue<st_TileBroadCast> m_queueBroadCast;
	std::vector<st_TileJob> m_vecJobBuffer;
	std::vector<st_TileBroadCast> m_vecBroadCastBuffer;

	CEntityVector m_vecPlayer;
	// 최초 변경 직전의 구성만 보존해 Tick 시작 AOI를 복원한다.
	bool m_bAoiSnapshotCaptured = false;
	std::vector<CEntity*> m_vecPreviousPlayers;

private:
	void TileJobRun();
	void TileBroadCast();
	void CaptureAoiSnapshot();

	void Broadcast(CPacket* pPacket, CEntity* pEntity = nullptr);

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
	void AppendCurrentPlayers(std::vector<CEntity*>& players) const;
	void AppendPreviousPlayers(std::vector<CEntity*>& players) const;
	void ClearAoiSnapshot();

	void Update();
};
