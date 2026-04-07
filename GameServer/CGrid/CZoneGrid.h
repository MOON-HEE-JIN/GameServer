#pragma once
#include "CGrid.h"
#include <unordered_map>

class CZoneGrid : public CGrid
{

private:
	std::unordered_map<BlockCoord, BlockData, BlockCoordHasher> Blocks;
	
private:
	st_Vector3D WorldToVoxel(const st_Vector3F& worldPos);
	st_Vector3F VoxelToWorld(const st_Vector3D& voxelPos);
	bool IsVailedVoxel(const st_Vector3D& voxelPos);

	BlockLookup GetBlockLookup(const st_Vector3D& voxelPos);
	int GetBlockBitIndex(const BlockLookup& blockLook);

	bool GetOccupancyBit(const BlockLookup& lookup);
	bool GetWalkableBit(const BlockLookup& bitIndex);
	
public:
	bool IsOccupancy(const st_Vector3F& worldPos);
	bool IsWalkable(const st_Vector3F& worldPos);

public:
	bool IsStandable(const st_Vector3F& pos);
	bool GetVoxelHeight(const st_Vector3F& pos, float& outHeight);
	bool FindStandablePosition(const st_Vector3F& pos, st_Vector3F& outPos);
	bool DebugFindStandablePosition(const st_Vector3F& pos, st_Vector3F& outPos);

	friend class CBinFileManager;
};