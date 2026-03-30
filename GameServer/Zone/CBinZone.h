#pragma once
#include "../CUtill/CBinFile.h"
#pragma pack(push, 1)
struct ZoneBounds
{
	int ZoneId = 0;
	float OriginX = 0;
	float OriginY = 0;
	float OriginZ = 0;

	float ExtentX = 0;
	float ExtentY = 0;
	float ExtentZ = 0;
	char ZoneName[64] = {};
};

struct Portal
{
	int PortalId = 0;
	int FromZoneId = 0;
	int ToZoneId = 0;

	float LocationX = 0;
	float LocationY = 0;
	float LocationZ = 0;

	float TargetX = 0;
	float TargetY = 0;
	float TargetZ = 0;
};

struct SpawnPoint
{
	int ZoneId = 0;
	int SpawnId = 0;
	int TemplateId = 0;
	float RespawnSeconds = 0;
	int MaxAlive = 0;
	int GroupId = 0;
	float SpawnRadiusCm = 0;

	float LocationX = 0;
	float LocationY = 0;
	float LocationZ = 0;
};

struct TriggerVolume
{
	int ZoneId = 0;
	int TriggerId = 0;

	float LocationX = 0;
	float LocationY = 0;
	float LocationZ = 0;

	int ParamCount = 0;
};

struct TriggerVolumeParam
{
	char Key[64] = {};
	char Value[64] = {};
};

#pragma pack(pop)

class CBinZone :
    public CBinFile
{
public:
	std::vector<ZoneBounds> m_vecZoneBounds;
	std::vector<Portal> m_vecPortals;
	std::vector<SpawnPoint> m_vecSpawnPoints;
	std::vector<TriggerVolume> m_vecTriggerVolumes;
	std::unordered_map<int, std::vector<TriggerVolumeParam>> m_mapTriggerVolumeParams;

	const std::vector<ZoneBounds>& GetZoneBoundsVector() const { return m_vecZoneBounds; }
	const std::vector<Portal>& GetPortalVector() const { return m_vecPortals; }
	const std::vector<SpawnPoint>& GetSpawnPointVector() const { return m_vecSpawnPoints; }
	const std::vector<TriggerVolume>& GetTriggerVolumeVector() const { return m_vecTriggerVolumes; }
	const std::unordered_map<int, std::vector<TriggerVolumeParam>>& GetTriggerVolumeParamMap() const { return m_mapTriggerVolumeParams; }

private:
	virtual bool ChunkToData() override;
};

