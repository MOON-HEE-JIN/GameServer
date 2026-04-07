#include "CZoneGrid.h"

st_Vector3D CZoneGrid::WorldToVoxel(const st_Vector3F& worldPos)
{
	float DivVoxel = 1.0f / m_fVoxelSize;

	return st_Vector3D
	{
		static_cast<int>(std::floorf((worldPos.X - m_stOrigin.X) * DivVoxel)),
		static_cast<int>(std::floorf((worldPos.Y - m_stOrigin.Y) * DivVoxel)),
		static_cast<int>(std::floorf((worldPos.Z - m_stOrigin.Z) * DivVoxel))
	};
}

st_Vector3F CZoneGrid::VoxelToWorld(const st_Vector3D& voxelPos)
{
	return st_Vector3F(m_stOrigin.X + (voxelPos.X + 0.5f) * m_fVoxelSize,
		m_stOrigin.Y + (voxelPos.Y + 0.5f) * m_fVoxelSize,
		m_stOrigin.Z + (voxelPos.Z + 0.5f) * m_fVoxelSize);
}

bool CZoneGrid::IsVailedVoxel(const st_Vector3D& voxelPos)
{
	return voxelPos.X >= 0 && voxelPos.X < m_stGridSize.X &&
		voxelPos.Y >= 0 && voxelPos.Y < m_stGridSize.Y &&
		voxelPos.Z >= 0 && voxelPos.Z < m_stGridSize.Z;
}

BlockLookup CZoneGrid::GetBlockLookup(const st_Vector3D& voxelPos)
{
	return BlockLookup
	{
		BlockCoord(voxelPos.X / m_iSparseBlockSize, voxelPos.Y / m_iSparseBlockSize, voxelPos.Z / m_iSparseBlockSize),
		voxelPos.X % m_iSparseBlockSize,
		voxelPos.Y % m_iSparseBlockSize,
		voxelPos.Z % m_iSparseBlockSize
	};
}

int CZoneGrid::GetBlockBitIndex(const BlockLookup& blockLook)
{
	return blockLook.LocalX + (m_iSparseBlockSize * blockLook.LocalY) + (m_iSparseBlockSize * m_iSparseBlockSize * blockLook.LocalZ);
}


bool CZoneGrid::GetOccupancyBit(const BlockLookup& lookup)
{
	int BitIndex = GetBlockBitIndex(lookup);

	int wordIndex = BitIndex >> 6; // 64비트 단위로 나누기
	int bitOffset = BitIndex & 63; // 나머지 비트 오프셋

	if (wordIndex < Blocks[lookup.Coord].OccupancyBits.size() && wordIndex > 0)
	{
		return (Blocks[lookup.Coord].OccupancyBits[wordIndex] & (1ULL << bitOffset)) != 0;
	}
	return false;
}

bool CZoneGrid::GetWalkableBit(const BlockLookup& lookup)
{
	int BitIndex = GetBlockBitIndex(lookup);

	int wordIndex = BitIndex >> 6; // 64비트 단위로 나누기
	int bitOffset = BitIndex & 63; // 나머지 비트 오프셋

	if (wordIndex < Blocks[lookup.Coord].WalkableBits.size() && wordIndex > 0)
	{
		return (Blocks[lookup.Coord].WalkableBits[wordIndex] & (1ULL << bitOffset)) != 0;
	}
	return false;
}

bool CZoneGrid::IsOccupancy(const st_Vector3F& worldPos)
{
	st_Vector3D voxelPos = WorldToVoxel(worldPos);

	// 유효한 보셀인지 확인
	if(!IsVailedVoxel(voxelPos))
		return true;

	BlockLookup blockLook = GetBlockLookup(voxelPos);
	if (Blocks.find(blockLook.Coord) == Blocks.end())
		return false;

	return GetOccupancyBit(blockLook);
}

bool CZoneGrid::IsWalkable(const st_Vector3F& worldPos)
{
	st_Vector3D voxelPos = WorldToVoxel(worldPos);

	// 유효한 보셀인지 확인
	if (!IsVailedVoxel(voxelPos))
		return false;

	BlockLookup blockLook = GetBlockLookup(voxelPos);
	if (Blocks.find(blockLook.Coord) == Blocks.end())
		return false;

	return GetWalkableBit(blockLook);
}

bool CZoneGrid::IsStandable(const st_Vector3F& pos)
{
	if (IsOccupancy(pos))
		return false;

	// 이후 조건문 추가 할수 있음

	return true;
}

bool CZoneGrid::GetVoxelHeight(const st_Vector3F& pos, float& outHeight)
{
	// 현재 위치에서 위로 레이캐스트를 수행하여 가장 가까운 충돌 지점을 찾는 로직을 구현해야 합니다.
	st_Vector3D voxelPos = WorldToVoxel(pos);

	// 유효한 보셀인지 확인
	if (!IsVailedVoxel(voxelPos))
		return false;

	int range = 5;

	int minZ = std::max(voxelPos.Z - range, 0);

	st_Vector3D checkVoxelPos = voxelPos;
	for (int Z = 0; Z <= minZ; Z++)
	{
		checkVoxelPos.Z = Z;
		st_Vector3F checkWorldPos = VoxelToWorld(checkVoxelPos);
		if (IsOccupancy(checkWorldPos))
		{
			outHeight = checkWorldPos.Z;
			return true;
		}
	}

	return false;
}

bool CZoneGrid::FindStandablePosition(const st_Vector3F& pos, st_Vector3F& outPos)
{
	st_Vector3D voxelPos = WorldToVoxel(pos);

	if (!IsVailedVoxel(voxelPos))
		return false;

	int findRange = 5;

	int minZ = std::max(voxelPos.Z - findRange, 0);
	int maxZ = std::min(voxelPos.Z + findRange, static_cast<int>(m_stGridSize.Z) - 1);

	st_Vector3D checkVoxelPos = voxelPos;

	for (int Z = minZ; Z <= maxZ; Z++)
	{
		checkVoxelPos.Z = Z;
		st_Vector3F checkWorldPos = VoxelToWorld(checkVoxelPos);
		if (IsStandable(checkWorldPos))
		{
			outPos = checkWorldPos;
			return true;
		}
	}
	return false;
}

bool CZoneGrid::DebugFindStandablePosition(const st_Vector3F& pos, st_Vector3F& outPos)
{
	st_Vector3D voxelPos = WorldToVoxel(pos);

	if (!IsVailedVoxel(voxelPos))
		return false;

	int findRange = 5;

	int minZ = 0;
	int maxZ = m_stGridSize.Z - 1;

	st_Vector3D checkVoxelPos = voxelPos;

	for (int Z = minZ; Z <= maxZ; Z++)
	{
		checkVoxelPos.Z = Z;
		st_Vector3F checkWorldPos = VoxelToWorld(checkVoxelPos);
		if (IsStandable(checkWorldPos))
		{
			outPos = checkWorldPos;
			return true;
		}
	}
	return false;
}

