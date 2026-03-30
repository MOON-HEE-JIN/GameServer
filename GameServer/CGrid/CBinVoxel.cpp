#include "CBinVoxel.h"
#include "../CUtill/CBinFileDefine.h"

bool CBinVoxel::ChunkToData()
{
	// ZONEBOUDNS
	{
		int ChunkTag = TZoneInfo;
		if (m_mapChunk.find(ChunkTag) == m_mapChunk.end())
		{
#ifdef __TEST_PROJECT_PRINTF__
			printf("Chunk with Tag 0x%X not found.\n", ChunkTag);
#endif
			return false;
		}

		std::string strChunkData = m_mapChunk[ChunkTag];

		int chunkDataSize = strChunkData.size();
		int offset = 0;
		int offsizefset = sizeof(BinZoneInfo);
		while (chunkDataSize > 0)
		{
			BinZoneInfo info = { 0 };
			memcpy(&info, strChunkData.c_str() + offset, offsizefset);
			m_Zone = info;
			offset += offsizefset;
			chunkDataSize -= offsizefset;
#ifdef __TEST_PROJECT_PRINTF__
			printf("\nZoneId: %d\n", info.ZoneId);
			printf("ZoneOriginX: %f\n", info.ZoneOriginX);
			printf("ZoneOriginY: %f\n", info.ZoneOriginY);
			printf("ZoneOriginZ: %f\n", info.ZoneOriginZ);

			printf("BoundsExtentMinX: %f\n", info.BoundsExtentMinX);
			printf("BoundsExtentMinY: %f\n", info.BoundsExtentMinY);
			printf("BoundsExtentMinZ: %f\n", info.BoundsExtentMinZ);

			printf("BoundsExtentMaxX: %f\n", info.BoundsExtentMaxX);
			printf("BoundsExtentMaxY: %f\n", info.BoundsExtentMaxY);
			printf("BoundsExtentMaxZ: %f\n", info.BoundsExtentMaxZ);
#endif
		}
	}

	// Grid
	{
		int ChunkTag = TGridInfo;
		if (m_mapChunk.find(ChunkTag) == m_mapChunk.end())
		{
#ifdef __TEST_PROJECT_PRINTF__
			printf("Chunk with Tag 0x%X not found.\n", ChunkTag);
#endif
			return false;
		}

		std::string strChunkData = m_mapChunk[ChunkTag];

		int chunkDataSize = strChunkData.size();
		int offset = 0;
		int offsizefset = sizeof(BinZoneGrid);
		while (chunkDataSize > 0)
		{
			BinZoneGrid info = { 0 };
			memcpy(&info, strChunkData.c_str() + offset, offsizefset);
			m_Grid = info;
			offset += offsizefset;
			chunkDataSize -= offsizefset;
#ifdef __TEST_PROJECT_PRINTF__
			printf("\nGridOriginX: %f\n", info.GridOriginX);
			printf("GridOriginY: %f\n", info.GridOriginY);
			printf("GridOriginZ: %f\n", info.GridOriginZ);

			printf("VoxelSize: %f\n", info.VoxelSize);

			printf("GridSizeX: %d\n", info.GridSizeX);
			printf("GridSizeY: %d\n", info.GridSizeY);
			printf("GridSizeZ: %d\n", info.GridSizeZ);

			printf("ChunkSizeX: %d\n", info.ChunkSizeX);
			printf("ChunkSizeY: %d\n", info.ChunkSizeY);
			printf("ChunkSizeZ: %d\n", info.ChunkSizeZ);
#endif
		}
	}

	// Build Info
	{
		int ChunkTag = TBuildInfo;
		if (m_mapChunk.find(ChunkTag) == m_mapChunk.end())
		{
#ifdef __TEST_PROJECT_PRINTF__
			printf("Chunk with Tag 0x%X not found.\n", ChunkTag);
#endif
			return false;
		}

		std::string strChunkData = m_mapChunk[ChunkTag];

		int chunkDataSize = strChunkData.size();
		int offset = 0;
		int offsizefset = sizeof(BinBuildInfo);
		while (chunkDataSize > 0)
		{
			BinBuildInfo info = { 0 };
			memcpy(&info, strChunkData.c_str() + offset, offsizefset);
			m_Build = info;
			offset += offsizefset;
			chunkDataSize -= offsizefset;
		}
#ifdef __TEST_PROJECT_PRINTF__

#endif
	}

	// Occupancy
	{
		int ChunkTag = TOccupancy;
		if (m_mapChunk.find(ChunkTag) == m_mapChunk.end())
		{
#ifdef __TEST_PROJECT_PRINTF__
			printf("Chunk with Tag 0x%X not found.\n", ChunkTag);
#endif
			return false;
		}

		std::string strChunkData = m_mapChunk[ChunkTag];

		int chunkDataSize = strChunkData.size();
		int offset = 0;
		int offsizefset = sizeof(__int8);
		while (chunkDataSize > 0)
		{
			__int8 bits = 0;
			memcpy(&bits, strChunkData.c_str() + offset, offsizefset);

			m_vecOccupancyData.push_back(bits);
			offset += offsizefset;
			chunkDataSize -= offsizefset;
		}
#ifdef __TEST_PROJECT_PRINTF__
			printf("\nOccupancyData Vector Size %lld\n", m_vecOccupancyData.size());
#endif
	}

	// Walkable Surface
	{
		int ChunkTag = TWalkable;
		if (m_mapChunk.find(ChunkTag) == m_mapChunk.end())
		{
#ifdef __TEST_PROJECT_PRINTF__
			printf("Chunk with Tag 0x%X not found.\n", ChunkTag);
#endif
			return false;
		}

		std::string strChunkData = m_mapChunk[ChunkTag];

		int chunkDataSize = strChunkData.size();
		int offset = 0;
		int offsizefset = sizeof(__int8);
		while (chunkDataSize > 0)
		{
			__int8 bits = 0;
			memcpy(&bits, strChunkData.c_str() + offset, offsizefset);

			m_vecWalkableData.push_back(bits);
			offset += offsizefset;
			chunkDataSize -= offsizefset;
		}
#ifdef __TEST_PROJECT_PRINTF__
			printf("\nWalkableData Vector Size %lld\n", m_vecWalkableData.size());

#endif
	}

	return true;
}
