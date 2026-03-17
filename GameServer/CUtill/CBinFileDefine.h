#pragma once

#define MAPTOOL_MAGIC 0xDEADBEEF
#define MAPTOOL_VERSION  1

struct FMT_BinFileTag
{
	unsigned int Tag;

	FMT_BinFileTag(unsigned char c1, unsigned char c2, unsigned char c3, unsigned char c4)
	{
		Tag = (c1 << 24) | (c2 << 16) | (c3 << 8) | c4;
	}
};

static const FMT_BinFileTag TAG_ZONEIDX('Z', 'I', 'D', 'X'); // Zone Index
static const FMT_BinFileTag TAG_ZONEBOUNDS('Z', 'B', 'N', 'D'); // Zone Bounds
static const FMT_BinFileTag TAG_PORTAL('P', 'O', 'R', 'T'); // Portal
static const FMT_BinFileTag TAG_SPAWN('S', 'P', 'W', 'N'); // Spawn Point
static const FMT_BinFileTag TAG_TRIGGER('T', 'R', 'I', 'G'); // Trigger Volume

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