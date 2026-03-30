#pragma once

#include "CZone.h"
#include <vector>
#include <unordered_map>
#include "../Zone/ZoneDefines.h"

class CZoneManager
{
public:
	CZoneManager();
	~CZoneManager();

public:
	bool ReadZoneBinFile(const char* filepath);

private:
	std::vector<CZone*> m_vecZone;
	std::unordered_map<int, st_IDX> m_mapZoneIDX;
	std::vector<CZone*> m_vecTempZone;// BinFile 에서 Zone 정보를 읽어올 때 임시로 저장하는 벡터 이후 m_vecZone 대체
	std::unordered_map<int, int> m_mapZoneIDtoIndex; // <ZoneID, m_vecZone Index>
	int m_maxZoneCnt;

private:
	bool TryEnterZone(int toZone);

public :
	void InsertZoneIDX(const st_IDX& idx) { m_mapZoneIDX[idx.ZoneId] = idx; }
public:
	int GetProcID(int zone);
	int GetMaxZoneCnt(){ return m_maxZoneCnt; }
	
	bool ReqEnterZone(CPlayer* pPlayer, int toZone);
	void ReqJob(ZONE_JOB& job, int zone);
	int InitProcZoneVector(int pid, std::vector<CZone*>& vec);

	bool IsValidZoneID(int zoneid) const;
	bool IsEqualProcZoneID(int from, int to);

	bool EnterZone(CPlayer* pPlayer, int zoneid);
	bool LeaveZone(CPlayer* pPlayer);
	void Log();
};

extern CZoneManager g_ZoneManager;
