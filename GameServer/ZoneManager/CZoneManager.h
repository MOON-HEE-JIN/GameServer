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

private:
	std::unordered_map<int, CZone*> m_mapZone;	// <ZoneID, Zone*>
	std::unordered_map<int, st_IDX> m_mapZoneIDX;
	
private:
	bool TryEnterZone(int toZone);

public:
	void InsertZone(CZone* pZone) { m_mapZone[pZone->GetID()] = pZone; }
	const std::unordered_map<int, st_IDX>& GetZoneIDXMap() { return m_mapZoneIDX; }

public:
	int GetProcID(int zone);
	CZone* GetZone(int zone) { return IsValidZoneID(zone) ? m_mapZone[zone] : nullptr; }

	bool ReqEnterZone(CPlayer* pPlayer, int toZone);
	void ReqJob(ZONE_JOB& job, int zone);
	int InitProcZoneVector(int pid, std::vector<CZone*>& vec);

	bool IsValidZoneID(int zoneid) const;
	bool IsEqualProcZoneID(int from, int to);

	bool EnterZone(CPlayer* pPlayer, int zoneid);
	bool LeaveZone(CPlayer* pPlayer);

	void PushZoneMoveVector(CEntity* pEntity);
	void PopZoneMoveVector(CEntity* pEntity);
public:
	void SendZone(int zone, CPacket* pPacket, CPlayer* pPlayer = nullptr);
	bool SendZoneInfo(int zone, CPlayer* pPlayer);
public:
	void Log();

	friend class CBinFileManager;
};

extern CZoneManager g_ZoneManager;
