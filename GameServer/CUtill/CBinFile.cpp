#include "CBinFile.h"
#include "CBinFileDefine.h"
#include "../Log/CLog.h"
CBinFile::CBinFile()
{
}

CBinFile::~CBinFile()
{
}

bool CBinFile::Open(const char* pszFileName)
{
	fopen_s(&fp, pszFileName, "rb");

	if (fp == nullptr)
		return false;

	return BinChunkRead();

}

void CBinFile::Close()
{
	if (fp != nullptr)
		fclose(fp);
}

bool CBinFile::BinChunkRead()
{
	FMT_BinFileHeader binFileHeader = { 0 };

	fread(&binFileHeader, sizeof(FMT_BinFileHeader), 1, fp);

	int CheckChunkCount = binFileHeader.ChunkCount;
	int offset = binFileHeader.ChunkHeaderOffset;
	if (binFileHeader.Magic != MAPTOOL_MAGIC)
	{
		g_LogFile.ELog("Invalid Magic Number: 0x%X != 0x%X", binFileHeader.Magic, MAPTOOL_MAGIC);
		return false;
	}
	if (binFileHeader.Version != MAPTOOL_VERSION)
	{
		g_LogFile.ELog("Unsupported Version: %d != %d", binFileHeader.Version, MAPTOOL_VERSION);
		return false;
	}

	for (int i = 0; i < CheckChunkCount; i++)
	{
		FMT_BinChunkHeader binChunkHeader = { 0 };
		fseek(fp, offset, SEEK_SET);
		fread(&binChunkHeader, sizeof(FMT_BinChunkHeader), 1, fp);

		offset = binChunkHeader.Offset;
		fseek(fp, offset, SEEK_SET);

		std::string strChunkData;
		strChunkData.resize(binChunkHeader.Size);

		fread(&strChunkData[0], sizeof(char), binChunkHeader.Size, fp);
		offset += binChunkHeader.Size;
		m_mapChunk[binChunkHeader.Tag] = strChunkData;
	}

	return ChunkToData();
}

