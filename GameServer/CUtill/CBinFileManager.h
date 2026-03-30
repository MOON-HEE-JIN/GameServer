#pragma once

class CBinFileManager
{
public:
	CBinFileManager();
	~CBinFileManager();
public:
	bool ReadBinZoneIdxFile(const char* filepath);
	bool ReadBinZoneFile(const char* filepath);
	bool ReadBinVoxelFile(const char* filepath);
};