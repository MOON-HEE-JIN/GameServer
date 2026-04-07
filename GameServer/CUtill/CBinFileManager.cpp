#include "CBinFileManager.h"

#include "../ZoneManager/CZoneManager.h"
#include "../Zone/CBinZoneIdx.h"
#include "../Zone/CBinZone.h"
#include "../CGrid/CBinVoxel.h"
#include "../CGrid/CZoneGrid.h"

CBinFileManager::CBinFileManager()
{
}

CBinFileManager::~CBinFileManager()
{
}

bool CBinFileManager::LoadBinFiles()
{
	std::string basePath = DATA_PATH_ROOT;

	std::string FilePath;

	FilePath = basePath + "/Zone/Zones.idx";
	if (!ReadBinZoneIdxFile(FilePath.c_str()))
		return false;
	std::unordered_map<int, st_IDX> mapZoneIDX = g_ZoneManager.GetZoneIDXMap();
	std::unordered_map<int, st_IDX>::iterator iter = mapZoneIDX.begin();
	std::unordered_map<int, st_IDX>::iterator eiter = mapZoneIDX.end();
	for (iter; iter != eiter; iter++)
	{
		const st_IDX& idx = iter->second;
		char name[128] = {};
		snprintf(name, sizeof(name), "Zone%03d", idx.ZoneId);

		FilePath = basePath + "/Zone/" + name + ".bin";
		if (!ReadBinZoneFile(FilePath.c_str()))
			return false;

		FilePath = basePath + "/Zone/" + name + ".vol";
		if (!ReadBinVoxelFile(FilePath.c_str()))
			return false;
	}

	return true;
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

		g_ZoneManager.m_mapZoneIDX[idx.ZoneId] = idx;
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
	std::vector<BinZoneBounds> vecZoneBounds = bin.GetZoneBoundsVector();

	CZone* pZone = new CZone(vecZoneBounds[0].ZoneId, procid, 2000, vecZoneBounds[0].ZoneName);

	{
		pZone->m_ZoneBounds.OriginX = vecZoneBounds[0].OriginX;
		pZone->m_ZoneBounds.OriginY = vecZoneBounds[0].OriginY;
		pZone->m_ZoneBounds.OriginZ = vecZoneBounds[0].OriginZ;
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
		
			pZone->m_vecPortal.push_back(portal);
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
			
			pZone->m_vecSpawnPoint.push_back(spawn);
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

			pZone->m_vecTriggerVolume.push_back(trigger);
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

			pZone->m_mapTriggerVolumeParams[triggerId] = vecStParam;
		}
	}

	g_ZoneManager.InsertZone(pZone);
	return true;
}

bool CBinFileManager::ReadBinVoxelFile(const char* filepath)
{
	CBinVoxel bin;

	if (!bin.Open(filepath))
		return false;
	
	CZoneGrid* pGrid = new CZoneGrid();
	pGrid->m_iID = bin.m_Zone.ZoneId;
	pGrid->m_fVoxelSize = bin.m_Grid.VoxelSize;

	pGrid->m_stOrigin(bin.m_Grid.GridOriginX, bin.m_Grid.GridOriginY, bin.m_Grid.GridOriginZ);
	pGrid->m_stMinExtent(bin.m_Zone.BoundsExtentMinX, bin.m_Zone.BoundsExtentMinY, bin.m_Zone.BoundsExtentMinZ);
	pGrid->m_stMaxExtent(bin.m_Zone.BoundsExtentMaxX, bin.m_Zone.BoundsExtentMaxY, bin.m_Zone.BoundsExtentMaxZ);
	pGrid->m_stGridSize(bin.m_Grid.GridSizeX, bin.m_Grid.GridSizeY, bin.m_Grid.GridSizeZ);

	//pGrid->PushBitData(eBitType::OCCUPANCY, bin.m_vecOccupancyData);
	//pGrid->PushBitData(eBitType::WALKABLE, bin.m_vecWalkableData);

	int Loop = bin.Blocks.size();
	for (int i = 0; i < Loop; i++)
	{
		const Block& block = bin.Blocks[i];
		BlockCoord coord(block.Info.BlockX, block.Info.BlockY, block.Info.BlockZ);
		BlockData data;
		data.OccupancyBits = block.Bits.CustomBits[eBitType::OCCUPANCY]; // 예시로 CustomBits[0]을 OccupancyBits로 사용
		data.WalkableBits = block.Bits.CustomBits[eBitType::WALKABLE];  // 예시로 CustomBits[1]을 WalkableBits로 사용
		
		pGrid->Blocks[coord] = data;
	}
	
	CZone* pZone = g_ZoneManager.GetZone(bin.m_Zone.ZoneId);
	if (pZone == nullptr)
		return false;

	pZone->SetZoneGrid(pGrid);

	return true;
}
