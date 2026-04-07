#pragma once

#define MAPTOOL_MAGIC 0xDEADBEEF
#define MAPTOOL_VERSION  1

static unsigned int MT_MAKE_TAG(char A, char B, char C, char D)
{
	return (static_cast<unsigned int>(A) << 24) |
		(static_cast<unsigned int>(B) << 16) |
		(static_cast<unsigned int>(C) << 8) |
		static_cast<unsigned int>(D);
}

static const unsigned int TZoneIDX = MT_MAKE_TAG('Z', 'I', 'D', 'X'); // Zone Index
static const unsigned int TZoneBounds = MT_MAKE_TAG('Z', 'B', 'N', 'D'); // Zone Bounds
static const unsigned int TPortal = MT_MAKE_TAG('P', 'O', 'R', 'T'); // Portal
static const unsigned int TSpawn = MT_MAKE_TAG('S', 'P', 'W', 'N'); // Spawn Point
static const unsigned int TTrigger = MT_MAKE_TAG('T', 'R', 'I', 'G'); // Trigger Volume
static const unsigned int TZoneInfo = MT_MAKE_TAG('Z', 'I', 'N', 'F'); // Zone Info
static const unsigned int TGridInfo = MT_MAKE_TAG('G', 'R', 'D', 'I'); // Grid Info
static const unsigned int TBuildInfo = MT_MAKE_TAG('B', 'L', 'D', 'I'); // Build Info
static const unsigned int TOccupancy = MT_MAKE_TAG('O', 'C', 'C', 'U'); // Voxel Data
static const unsigned int TWalkable = MT_MAKE_TAG('W', 'A', 'L', 'K'); // Walkable Surface
static const unsigned int TSparseBlocks = MT_MAKE_TAG('S', 'P', 'B', 'L'); // Sparse Voxel Blocks

#pragma pack(push, 1) // 구조체의 멤버들이 1바이트 단위로 정렬되도록 설정
struct FMT_BinFileHeader
{
	unsigned int Magic = 0; // 파일 식별용 매직 넘버
	int Version = 1; // 파일 포맷 버전
	int Endian = 0; // 0 = Little Endian, 1 = Big Endian
	int ChunkCount = 0; // 파일에 포함된 청크의 수
	int ChunkHeaderOffset = 0; // 청크 헤더가 시작되는 파일 내 오프셋
};

struct FMT_BinChunkHeader
{
	int Tag = 0; // 청
	int Offset = 0; // 청크 데이터가 시작되는 파일 내 오프셋
	int Size = 0; // 청크 데이터의 크기 (바이트 단위)
	int Count = 0; // 청크 내 요소의 수 (예: 포탈 청크의 경우 포탈 개수)
	int CRC32 = 0; // 청크 데이터의 CRC32 체크섬
};
#pragma pack(pop) // 구조체 정렬 설정을 원래대로 되돌림