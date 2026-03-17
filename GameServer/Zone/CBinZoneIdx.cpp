#include "CBinZoneIdx.h"
#include "../CUtill/CBinFileDefine.h"
#include "../Log/CLog.h"

bool CBinZoneIdx::ChunkToData()
{
	int ChunkTag = TAG_ZONEIDX.Tag;

	if (m_mapChunk.find(ChunkTag) == m_mapChunk.end())
	{
		g_LogFile.ELog("Chunk with Tag 0x%X not found.", ChunkTag);
		return false;
	}

	std::string strChunkData = m_mapChunk[ChunkTag];

	int chunkDataSize = strChunkData.size();
	int offset = 0;
	int offsizefset = sizeof(IDX);
	while (chunkDataSize > 0)
	{
		IDX idx = { 0 };
		memcpy(&idx, strChunkData.c_str() + offset, offsizefset);
		m_vecZoneIdx.push_back(idx);
		offset += offsizefset;

		chunkDataSize -= offsizefset;
	
		g_LogFile.ILog("ZoneIdx Chunk Read: ZoneId=%d, FileSizeBytes=%u, Crc32=%u, ZoneName=%s",
			idx.ZoneId, idx.FileSizeBytes, idx.Crc32, idx.ZoneName);
	}

	return true;
}
