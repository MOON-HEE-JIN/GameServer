#include "CGrid.h"

#include <cmath>

CGrid::CGrid()
{
	int TypeCount = static_cast<int>(eBitType::MAX_BIT_TYPE);

	m_vecBitData.resize(TypeCount);
}

CGrid::~CGrid()
{
}


__int64 CGrid::GetBitIndex(const st_Vector3D& position)
{
	return static_cast<__int64>(position.Z) * m_stGridSize.X * m_stGridSize.Y +
		static_cast<__int64>(position.Y) * m_stGridSize.X +
		static_cast<__int64>(position.X);
}


st_Vector3D CGrid::WorldToGrid(const st_Vector3F& pos)
{
	st_Vector3D gridPos;

	float DivVoxel = 1.0f / m_fVoxelSize;
	
	// 반올림 하여 girdPos 기준 좌우 voxel 위치 확인 가능하도록 함
	gridPos.X = static_cast<int>(std::floorf((pos.X - m_stOrigin.X) * DivVoxel));
	gridPos.Y = static_cast<int>(std::floorf((pos.Y - m_stOrigin.Y) * DivVoxel));
	gridPos.Z = static_cast<int>(std::floorf((pos.Z - m_stOrigin.Z) * DivVoxel));

	return gridPos;
}

st_Vector3F CGrid::GridToWorld(const st_Vector3D& gridPos)
{
	st_Vector3F worldPos;
	// gridPos 기준 voxel 중앙 위치 반환
	worldPos.X = m_stOrigin.X + (gridPos.X + 0.5f) * m_fVoxelSize;
	worldPos.Y = m_stOrigin.Y + (gridPos.Y + 0.5f) * m_fVoxelSize;
	worldPos.Z = m_stOrigin.Z + (gridPos.Z + 0.5f) * m_fVoxelSize;

	return worldPos;
}

bool CGrid::GetBit(eBitType type, const st_Vector3F& position)
{
	st_Vector3D gridPos = WorldToGrid(position);
	__int64 bitIndex = GetBitIndex(gridPos);
	int byteIndex = static_cast<int>(bitIndex >> 3);
	
	if (byteIndex >= m_vecBitData[type].size())
		return false;
	
	unsigned __int8 mask = 1u << (bitIndex & 7);

	return (m_vecBitData[type][byteIndex] & mask);
}

bool CGrid::GetBit(eBitType type, const st_Vector3D& position)
{
	st_Vector3D gridPos = position;
	__int64 bitIndex = GetBitIndex(gridPos);
	int byteIndex = static_cast<int>(bitIndex >> 3);

	if (byteIndex >= m_vecBitData[type].size())
		return false;
	
	unsigned __int8 mask = 1u << (bitIndex & 7);

	return (m_vecBitData[type][byteIndex] & mask);
}