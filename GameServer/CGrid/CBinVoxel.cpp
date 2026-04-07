#include "CBinVoxel.h"
#include "../CUtill/CBinFileDefine.h"
#include "../Log/CLog.h"

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

			g_LogFile.ILog("ZoneId: %d, ZoneOrigin: (%f, %f, %f), BoundsExtentMin: (%f, %f, %f), BoundsExtentMax: (%f, %f, %f)",
				info.ZoneId,
				info.ZoneOriginX, info.ZoneOriginY, info.ZoneOriginZ,
				info.BoundsExtentMinX, info.BoundsExtentMinY, info.BoundsExtentMinZ,
				info.BoundsExtentMaxX, info.BoundsExtentMaxY, info.BoundsExtentMaxZ);
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

			g_LogFile.ILog("GridOrigin: (%f, %f, %f), VoxelSize: %f, GridSize: (%d, %d, %d), ChunkSize: (%d, %d, %d)",
				info.GridOriginX, info.GridOriginY, info.GridOriginZ,
				info.VoxelSize,
				info.GridSizeX, info.GridSizeY, info.GridSizeZ,
				info.ChunkSizeX, info.ChunkSizeY, info.ChunkSizeZ);
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

#ifdef __READ_ALL_VOXEL__
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
			unsigned __int8 bits = 0;
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
			unsigned __int8 bits = 0;
			memcpy(&bits, strChunkData.c_str() + offset, offsizefset);

			m_vecWalkableData.push_back(bits);
			offset += offsizefset;
			chunkDataSize -= offsizefset;
		}
#ifdef __TEST_PROJECT_PRINTF__
		printf("\nWalkableData Vector Size %lld\n", m_vecWalkableData.size());

#endif
	}
#endif // __READ_ALL_VOXEL__

	// Sparse Block
	{
		unsigned int ChunkTag = TSparseBlocks;
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
		int offsizefset = sizeof(BinSparseBlock);
		while (chunkDataSize > 0)
		{
			Block block;
			memcpy(&block.Info, strChunkData.c_str() + offset, offsizefset);
			offset += offsizefset;

			int LoopVoxelType = block.Info.VoxelCount;
			int LoopWord = block.Info.WordCount;
			block.Bits.CustomBits.resize(LoopVoxelType);

			for (int i = 0; i < LoopVoxelType; i++)
			{
				for (int j = 0; j < LoopWord; j++)
				{
					unsigned __int64 Bits;
					memcpy(&Bits, strChunkData.c_str() + offset, sizeof(Bits));
					offset += sizeof(Bits);
					block.Bits.CustomBits[i].push_back(Bits);
				}
			}
			Blocks.push_back(block);
			chunkDataSize -= offsizefset + (sizeof(__int64) * LoopVoxelType * LoopWord);
		}
#ifdef __TEST_PROJECT_PRINTF__
		printf("\nOccupancyData Vector Size %lld\n", m_vecOccupancyData.size());
#endif

	}
	return true;
}