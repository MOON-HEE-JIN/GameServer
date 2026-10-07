#pragma once

#include "../Zone/CZone.h"
#include "../Zone/CZoneBasic.h"

#include <vector>
#include <unordered_map>
#include "../Zone/ZoneDefines.h"
#include "../MainWorld/CMainWorld.h"

class CZoneManager
{
public:
	CZoneManager();
	~CZoneManager();

public:
	bool ReadZoneBinFile(const char* filepath);
private:
	const int MAX_MAIN_WORLD_COUNT = 1;
	std::unordered_map<int, st_IDX> m_mapZoneIDX;
	std::vector<CZone*> m_vecTempZone;// BinFile 에서 Zone 정보를 읽어올 때 임시로 저장하는 벡터 이후 m_vecZone 대체
	std::unordered_map<int, int> m_mapZoneIDtoIndex; // <ZoneID, m_vecZone Index>
	int m_maxZoneCnt = 0;
	
	std::unordered_map<int, std::vector<CZoneBasic*>> m_mapZones; // [ZoneID][Channel,CZone]
private:
	bool TryEnterZone(int Channel, int toZone);
public :
	void StartMainWorld();
	void InsertZoneIDX(const st_IDX& idx) { m_mapZoneIDX[idx.ZoneId] = idx; }

public:
	int GetProcID(int Channel, int Zone);
	int GetMaxZoneCnt(){ return m_maxZoneCnt; }
	
	bool ReqEnterLoginZone(CPlayer* pPlayer);
	bool ReqEnterZone(CPlayer* pPlayer, int Channel, int ToZone);
	//void ReqJob(ZONE_CHANGE_JOB& job, int zone);
	int InitProcZoneVector(int pid, std::vector<CZoneBasic*>& vec);

	bool IsValidZoneID(int zoneid) const;
	bool IsValidChannelZone(int Channel, int ZoneID);
	bool IsEqualProcZoneID(int fromChannel, int from, int toChannel, int to);

	bool LeaveZone(CPlayer* pPlayer);

	bool PushZoneMoveVector(CEntity* pEntity);
	void PopZoneMoveVector(CEntity* pEntity);

	CZoneBasic* GetZone(int Channel, int ZoneID);
public:
	void SendZone(int Channel, int Zone, CPacket* pPacket, COORDINATE pivot, CPlayer* pPlayer = nullptr);
public:
	ULONGLONG m_iLogDelayTime = 1 * 1000;
	ULONGLONG m_iLogTime = 0;
	void Log();

};

bool EnqueueChangeJob(int id, int zone, ZONE_CHANGE_JOB& job);
extern CZoneManager g_ZoneManager;
