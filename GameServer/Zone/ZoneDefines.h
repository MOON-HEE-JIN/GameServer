#pragma once

struct st_IDX
{
	int ZoneId;
	unsigned int FileSizeBytes;
	unsigned int Crc32;
	char ZoneName[64];
};

struct st_ZoneBounds
{
	int ZoneId;
	float OriginX;
	float OriginY;
	float OriginZ;
	float ExtentX;
	float ExtentY;
	float ExtentZ;
	char ZoneName[64];
};

struct st_Portal
{
	int PortalId;
	int FromZoneId;
	int ToZoneId;
	float LocationX;
	float LocationY;
	float LocationZ;
	float TargetX;
	float TargetY;
	float TargetZ;
};

struct st_SpawnPoint
{
	int ZoneId;
	int SpawnId;
	int TemplateId;
	float RespawnSeconds;
	int MaxAlive;
	int GroupId;
	float SpawnRadiusCm;
	float LocationX;
	float LocationY;
	float LocationZ;
};

struct st_TriggerVolumeParam
{
	char Key[64] = {};
	char Value[64] = {};
};

struct st_TriggerVolume
{
	int ZoneId;
	int TriggerId;
	float LocationX;
	float LocationY;
	float LocationZ;
	int ParamCount;
};