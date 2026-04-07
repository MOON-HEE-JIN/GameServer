#pragma once
#include "../CUtill/CBinFile.h"

#pragma pack(push, 1)
struct BinZoneInfo
{
	int ZoneId;

	float ZoneOriginX;
	float ZoneOriginY;
	float ZoneOriginZ;

	float BoundsExtentMinX;
	float BoundsExtentMinY;
	float BoundsExtentMinZ;

	float BoundsExtentMaxX;
	float BoundsExtentMaxY;
	float BoundsExtentMaxZ;
};

struct BinZoneGrid
{
	float GridOriginX;
	float GridOriginY;
	float GridOriginZ;

	float VoxelSize;
	int SparseBlockSize;

	int GridSizeX;
	int GridSizeY;
	int GridSizeZ;

	int ChunkSizeX;
	int ChunkSizeY;
	int ChunkSizeZ;
};

struct BinBuildInfo
{
	bool bEnableConservativeRaster;				// 보수적 래스터화(Conservative Rasterization)를 활성화하여, 삼각형이 Voxel의 경계에 걸치는 경우에도 해당 Voxel을 솔리드로 간주하여 기록하는 옵션. 이를 통해 래스터화 과정에서 발생할 수 있는 누락된 솔리드 Voxel을 줄일 수 있음.
	bool bTreatLandscapeAsSurfaceOnly;			// Landscape를 표면으로만 처리하여, 래스터화 과정에서 Landscape 내부의 Voxel이 솔리드로 간주되지 않도록 하는 옵션. 이를 통해 Landscape의 지형 형태를 보다 정확하게 표현할 수 있음.
	bool bEnableLandscapeDownFill;				// Landscape에서 추출된 삼각형이 래스터화된 후, 래스터화된 Voxel 아래의 Voxel들도 솔리드로 간주하여 채우는 옵션. 이를 통해 래스터화 과정에서 발생할 수 있는 누락된 솔리드 Voxel을 줄일 수 있음.
	bool bEnableLandscapeFillDepthLimit;		// Landscape에서 추출된 삼각형이 래스터화된 후, 래스터화된 Voxel 아래의 Voxel들도 솔리드로 간주하여 채울 때, 최대 채우기 깊이를 설정하는 옵션. 이를 통해 너무 깊은 Voxel까지 채워지는 것을 방지하여 래스터화의 효율성을 높일 수 있음.

	float TriangleAABBInflateCm;			// 삼각형의 Axis-Aligned Bounding Box(AABB)를 확장하는 크기 (cm 단위)로, 래스터화 과정에서 삼각형이 Voxel의 경계에 걸치는 경우에도 해당 Voxel을 솔리드로 간주하여 기록하는 옵션. 이를 통해 래스터화 과정에서 발생할 수 있는 누락된 솔리드 Voxel을 줄일 수 있음.
	float ConservativeExpandCm;				// 보수적 래스터화에서 삼각형이 Voxel의 경계에 걸치는 경우, 해당 Voxel을 솔리드로 간주하여 기록할 때, 삼각형의 AABB를 확장하는 크기 (cm 단위)로, 래스터화 과정에서 발생할 수 있는 누락된 솔리드 Voxel을 줄일 수 있음.
	float LandscapeDownFillBottomZ;			// Landscape에서 추출된 삼각형이 래스터화된 후, 래스터화된 Voxel 아래의 Voxel들도 솔리드로 간주하여 채울 때, 채우기의 하한 Z값 (cm 단위)으로, 이 값보다 낮은 Z값을 가진 Voxel은 채우지 않도록 하여 래스터화의 효율성을 높일 수 있음.
	float LandscapeDownFillMaxDepth;		// Landscape에서 추출된 삼각형이 래스터화된 후, 래스터화된 Voxel 아래의 Voxel들도 솔리드로 간주하여 채울 때, 최대 채우기 깊이 (cm 단위)로, 너무 깊은 Voxel까지 채워지는 것을 방지하여 래스터화의 효율성을 높일 수 있음.

	// Walkable Surface 관련 옵션
	float WalkableSlopeAngle;				// 걸을 수 있는 표면으로 간주할 최대 경사각 (도 단위)으로, 래스터화 과정에서 삼각형의 법선 벡터와 월드 업 벡터 사이의 각도가 이 값보다 작거나 같은 경우, 해당 삼각형이 걸을 수 있는 표면으로 간주되어 WalkableMask에 기록될 수 있음.
	float WalkableHeightCm;					// 캐릭터가 걸을 수 있는 최소 높이 (cm 단위)로, 래스터화 과정에서 삼각형이 걸을 수 있는 표면으로 간주되기 위해서는, 해당 삼각형의 높이가 이 값보다 크거나 같아야 함. 이를 통해 너무 낮은 표면이 걸을 수 있는 표면으로 간주되는 것을 방지할 수 있음.

	bool bRequireHeadroom;

	bool bRequireSupportBelow;
	bool bUseSlopFilter;
};


struct BinSparseBlock
{
	int BlockX;
	int BlockY;
	int BlockZ;

	int VoxelCount;
	int WordCount;
};

struct BlockBits
{
	std::vector<std::vector<unsigned __int64>> CustomBits; // 향후 확장용
	int Size = 8;
	bool GetOccupancy(int X, int Y, int Z)
	{
		int BitIndex = (X + (Size * Y) + (Size * Size * Z));
		int WordIndex = BitIndex >> 6;
		int BitOffset = BitIndex & 63;
		return (CustomBits[0][WordIndex] & (__int64(1) << BitOffset)) != 0;
	}

	bool GetWalkable(int X, int Y, int Z)
	{
		int BitIndex = (X + (Size * Y) + (Size * Size * Z));
		int WordIndex = BitIndex >> 6;
		int BitOffset = BitIndex & 63;
		return (CustomBits[1][WordIndex] & (__int64(1) << BitOffset)) != 0;
	}
};

struct Block
{
	BinSparseBlock Info;
	BlockBits Bits;
};
#pragma pack(pop)

class CBinVoxel :
    public CBinFile
{
public:
	BinZoneInfo m_Zone;
	BinZoneGrid m_Grid;
	BinBuildInfo m_Build;
	
	std::vector<unsigned __int8> m_vecOccupancyData;
	std::vector<unsigned __int8> m_vecWalkableData;
	
	std::vector<Block> Blocks;
public:
	bool ChunkToData() override;
};

