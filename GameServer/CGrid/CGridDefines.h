#pragma once
#include <vector>

enum eBitType
{
	OCCUPANCY,	// 충돌 비트
	WALKABLE,	// 이동 가능 비트
	MAX_BIT_TYPE
};

struct BlockCoord
{
	int X;
	int Y;
	int Z;
	BlockCoord() : X(0), Y(0), Z(0) {}
	BlockCoord(int x, int y, int z) : X(x), Y(y), Z(z) {}

	bool operator==(const BlockCoord& other) const
	{
		return X == other.X && Y == other.Y && Z == other.Z;
	}
};

struct BlockCoordHasher
{
	std::size_t operator()(const BlockCoord& coord) const
	{
		std::size_t h1 = std::hash<int>()(coord.X);
		std::size_t h2 = std::hash<int>()(coord.Y);
		std::size_t h3 = std::hash<int>()(coord.Z);
		return h1 ^ (h2 << 1) ^ (h3 << 2); // 간단한 해시 조합
	}
};

struct BlockData
{
	std::vector<unsigned __int64> OccupancyBits; // 충돌 비트 데이터
	std::vector<unsigned __int64> WalkableBits;  // 이동 가능 비트 데이터
};

struct BlockLookup
{
	BlockCoord Coord;
	int LocalX;
	int LocalY;
	int LocalZ;
};