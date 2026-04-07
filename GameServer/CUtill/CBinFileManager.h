#pragma once

#define DATA_PATH_ROOT "D:/GameServer/Data"

class CBinFileManager
{
public:
	CBinFileManager();
	~CBinFileManager();
public:
	bool LoadBinFiles();
private:
	bool ReadBinZoneIdxFile(const char* filepath);
	bool ReadBinZoneFile(const char* filepath);
	bool ReadBinVoxelFile(const char* filepath);
};