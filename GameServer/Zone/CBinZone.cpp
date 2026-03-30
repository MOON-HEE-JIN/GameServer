#include "CBinZone.h"
#include "../CUtill/CBinFileDefine.h"
#include "../Log/CLog.h"

bool CBinZone::ChunkToData()
{
	// ZONEBOUDNS
	{
		int ChunkTag = TZoneBounds;
		if (m_mapChunk.find(ChunkTag) == m_mapChunk.end())
		{
			g_LogFile.ELog("Chunk with ZoneBounds Tag 0x%X not found.", ChunkTag);
			return false;
		}

		std::string strChunkData = m_mapChunk[ChunkTag];

		int chunkDataSize = strChunkData.size();
		int offset = 0;
		int offsizefset = sizeof(BinZoneBounds);
		while (chunkDataSize > 0)
		{
			BinZoneBounds zoneBounds = { 0 };
			memcpy(&zoneBounds, strChunkData.c_str() + offset, offsizefset);
			m_vecZoneBounds.push_back(zoneBounds);
			offset += offsizefset;
			chunkDataSize -= offsizefset;
		}
	}

	// PORTAL
	{
		int ChunkTag = TPortal;
		if (m_mapChunk.find(ChunkTag) == m_mapChunk.end())
		{
			g_LogFile.ELog("Chunk with Portal Tag 0x%X not found.", ChunkTag);
			return false;
		}
		std::string strChunkData = m_mapChunk[ChunkTag];
		int chunkDataSize = strChunkData.size();
		int offset = 0;
		int offsizefset = sizeof(BinPortal);
		while (chunkDataSize > 0)
		{
			BinPortal portal = { 0 };
			memcpy(&portal, strChunkData.c_str() + offset, offsizefset);
			m_vecPortals.push_back(portal);
			offset += offsizefset;
			chunkDataSize -= offsizefset;
		}
	}

	// SPAWN
	{
		int ChunkTag = TSpawn;
		if (m_mapChunk.find(ChunkTag) == m_mapChunk.end())
		{
			g_LogFile.ELog("Chunk with Spawn Tag 0x%X not found.", ChunkTag);
			return false;
		}
		std::string strChunkData = m_mapChunk[ChunkTag];
		int chunkDataSize = strChunkData.size();
		int offset = 0;
		int offsizefset = sizeof(BinSpawnPoint);
		while (chunkDataSize > 0)
		{
			BinSpawnPoint spawnPoint = { 0 };
			memcpy(&spawnPoint, strChunkData.c_str() + offset, offsizefset);
			m_vecSpawnPoints.push_back(spawnPoint);
			offset += offsizefset;
			chunkDataSize -= offsizefset;
		}
	}

	// TRIGGER
	{
		int ChunkTag = TTrigger;
		if (m_mapChunk.find(ChunkTag) == m_mapChunk.end())
		{
			g_LogFile.ELog("Chunk with Trigger Tag 0x%X not found.", ChunkTag);
			return false;
		}
		std::string strChunkData = m_mapChunk[ChunkTag];
		int chunkDataSize = strChunkData.size();
		int offset = 0;
		int offsizefset = sizeof(BinTriggerVolume);
		while (chunkDataSize > 0)
		{
			BinTriggerVolume triggerVolume = { 0 };
			memcpy(&triggerVolume, strChunkData.c_str() + offset, offsizefset);
			m_vecTriggerVolumes.push_back(triggerVolume);
			offset += offsizefset;
			chunkDataSize -= offsizefset;

			int paramCount = triggerVolume.ParamCount;
			for (size_t i = 0; i < paramCount; i++)
			{
				BinTriggerVolumeParam param = { 0 };
				memcpy(&param, strChunkData.c_str() + offset, sizeof(BinTriggerVolumeParam));
				offset += sizeof(BinTriggerVolumeParam);
				chunkDataSize -= sizeof(BinTriggerVolumeParam);
				
				m_mapTriggerVolumeParams[triggerVolume.TriggerId].push_back(param);
			}
		}
	}

    return false;
}
