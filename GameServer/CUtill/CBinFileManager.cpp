#include "CBinFileManager.h"

#include "../ZoneManager/CZoneManager.h"
#include "../Zone/CBinZoneIdx.h"
#include "../Zone/CBinZone.h"

CBinFileManager::CBinFileManager()
{
}

CBinFileManager::~CBinFileManager()
{
}

bool CBinFileManager::ReadBinZoneIdxFile(const char* filepath)
{
	CBinZoneIdx bin;

	if (!bin.Open(filepath))
		return false;
	
	const std::vector<IDX> vecZoneIdx = bin.GetZoneIdxVector();
	int Loop = vecZoneIdx.size();
	for (int i = 0; i < Loop; i++)
	{
		const IDX& data = vecZoneIdx[i];
		st_IDX idx;
		idx.ZoneId = data.ZoneId;
		idx.FileSizeBytes = data.FileSizeBytes;
		idx.Crc32 = data.Crc32;
		strcpy_s(idx.ZoneName, data.ZoneName);
		g_ZoneManager.InsertZoneIDX(idx);
	}
	return true;
}

bool CBinFileManager::ReadBinZoneFile(const char* filepath)
{
	CBinZone bin;

	if (!bin.Open(filepath))
		return false;

	static int Index = 1;

	int procid = (Index + 1) % (ProcThreadCnt - 1) + 1;	// 1 ~ 2 까지
	CZone* pZone = new CZone(0,Index, procid, 2000);

	std::vector<BinZoneBounds> vecZoneBounds = bin.GetZoneBoundsVector();
	{
		st_ZoneBounds zoneBounds;
		zoneBounds.ZoneId = vecZoneBounds[0].ZoneId;
		zoneBounds.OriginX = vecZoneBounds[0].OriginX;
		zoneBounds.OriginY = vecZoneBounds[0].OriginY;
		zoneBounds.OriginZ = vecZoneBounds[0].OriginZ;
		zoneBounds.ExtentX = vecZoneBounds[0].ExtentX;
		zoneBounds.ExtentY = vecZoneBounds[0].ExtentY;
		zoneBounds.ExtentZ = vecZoneBounds[0].ExtentZ;
		strcpy_s(zoneBounds.ZoneName, vecZoneBounds[0].ZoneName);
	}
	
	std::vector<BinPortal> vecPortal = bin.GetPortalVector();
	{
		int Loop = vecPortal.size();
		for (int i = 0; i < Loop; i++)
		{
			st_Portal portal;

			portal.PortalId = vecPortal[i].PortalId;
			portal.FromZoneId = vecPortal[i].FromZoneId;
			portal.ToZoneId = vecPortal[i].ToZoneId;
			portal.LocationX = vecPortal[i].LocationX;
			portal.LocationY = vecPortal[i].LocationY;
			portal.LocationZ = vecPortal[i].LocationZ;
			portal.TargetX = vecPortal[i].TargetX;
			portal.TargetY = vecPortal[i].TargetY;
			portal.TargetZ = vecPortal[i].TargetZ;
		
			pZone->InsertPortal(portal);
		}
	}

	std::vector<BinSpawnPoint> vecSpawnPoint = bin.GetSpawnPointVector();
	{
		int Loop = vecSpawnPoint.size();
		for (int i = 0; i < Loop; i++)
		{
			st_SpawnPoint spawn;
			spawn.ZoneId = vecSpawnPoint[i].ZoneId;
			spawn.SpawnId = vecSpawnPoint[i].SpawnId;
			spawn.TemplateId = vecSpawnPoint[i].TemplateId;
			spawn.RespawnSeconds = vecSpawnPoint[i].RespawnSeconds;
			spawn.MaxAlive = vecSpawnPoint[i].MaxAlive;
			spawn.GroupId = vecSpawnPoint[i].GroupId;
			spawn.SpawnRadiusCm = vecSpawnPoint[i].SpawnRadiusCm;
			spawn.LocationX = vecSpawnPoint[i].LocationX;
			spawn.LocationY = vecSpawnPoint[i].LocationY;
			spawn.LocationZ = vecSpawnPoint[i].LocationZ;
			pZone->InsertSpawnPoint(spawn);
		}
	}

	std::vector<BinTriggerVolume> vecTriggerVolume = bin.GetTriggerVolumeVector();
	{
		int Loop = vecTriggerVolume.size();
		for (int i = 0; i < Loop; i++)
		{
			st_TriggerVolume trigger;
			trigger.ZoneId = vecTriggerVolume[i].ZoneId;
			trigger.TriggerId = vecTriggerVolume[i].TriggerId;
			trigger.LocationX = vecTriggerVolume[i].LocationX;
			trigger.LocationY = vecTriggerVolume[i].LocationY;
			trigger.LocationZ = vecTriggerVolume[i].LocationZ;
			trigger.ParamCount = vecTriggerVolume[i].ParamCount;
			pZone->InsertTriggerVolume(trigger);
		}
	}

	std::unordered_map<int, std::vector<BinTriggerVolumeParam>> mapTriggerVolumeParam = bin.GetTriggerVolumeParamMap();
	{
		std::unordered_map<int, std::vector<BinTriggerVolumeParam>>::iterator iter = mapTriggerVolumeParam.begin();
		std::unordered_map<int, std::vector<BinTriggerVolumeParam>>::iterator eiter = mapTriggerVolumeParam.end();
		
		for (iter; iter != eiter; iter++)
		{
			int triggerId = iter->first;
			std::vector<BinTriggerVolumeParam> vecParam = iter->second;
			std::vector<st_TriggerVolumeParam> vecStParam;
			int Loop = vecParam.size();
			for (int i = 0; i < Loop; i++)
			{
				st_TriggerVolumeParam stParam;
				strcpy_s(stParam.Key, vecParam[i].Key);
				strcpy_s(stParam.Value, vecParam[i].Value);
				vecStParam.push_back(stParam);
			}
			pZone->InsertTriggerVolumeParam(triggerId, vecStParam);
		}
	}

	return true;
}

bool CBinFileManager::ReadBinVoxelFile(const char* filepath)
{
	
	return false;
}
