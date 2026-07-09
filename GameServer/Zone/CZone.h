#pragma once
#include "../CPlayer.h"
#include <vector>
#include <unordered_map>
#include <atomic>

#include "../Zone/CZoneBasic.h"
#include "../Zone/ZoneDefines.h"

class CZone : public CZoneBasic
{
public:
	CZone(int channel, int ZoneID, int ProcID, int Maximum);
	~CZone();
	
protected:
	
	st_ZoneBounds m_ZoneBounds;				// Zone 의 Bounds 정보
	std::vector<st_Portal> m_vecPortal;		// Zone 의 Portal 정보
	std::vector<st_SpawnPoint> m_vecSpawnPoint;	// Zone 의 SpawnPoint 정보
	std::vector<st_TriggerVolume> m_vecTriggerVolume;	// Zone 의 TriggerVolume 정보
	std::unordered_map<int, std::vector<st_TriggerVolumeParam>> m_mapTriggerVolumeParams;	// <TriggerId, TriggerVolumeParam Vector>

	std::vector<CEntity*> m_vecEntityMoveVector;			// 움직임 전용 Update Vector
public:
	virtual void Process() override;
private:
	void ZoneEntityMoveProcess();	// Entity Position 이동 처리
public:
	void InitZone(const st_ZoneBounds& bounds) { m_ZoneBounds = bounds; }
	void InsertPortal(st_Portal portal) { m_vecPortal.push_back(portal); }
	void InsertSpawnPoint(st_SpawnPoint spawn) { m_vecSpawnPoint.push_back(spawn); }
	void InsertTriggerVolume(st_TriggerVolume trigger) { m_vecTriggerVolume.push_back(trigger); }
	void InsertTriggerVolumeParam(int index, std::vector<st_TriggerVolumeParam> param) { m_mapTriggerVolumeParams[index] = param; }

public:
	virtual bool PushMoveVector(CEntity* pEntity) override;
	virtual void PopMoveVector(CEntity* pEntity) override;

	virtual void OnLeaveZone(CPlayer* pPlayer) override;
};
