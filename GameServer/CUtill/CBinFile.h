#pragma once

#include <stdio.h>
#include <unordered_map>
#include <vector>
#include <string>
class CBinFile
{
public:
	CBinFile();
	~CBinFile();

	bool Open(const char* pszFileName);
	void Close();
protected:
	FILE* fp;
	std::unordered_map<int, std::string> m_mapChunk;

private:
	bool BinChunkRead();
	virtual bool ChunkToData() = 0;
};

