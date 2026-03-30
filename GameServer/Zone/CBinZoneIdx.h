#pragma once

#include "../CUtill/CBinFile.h"
#pragma pack(push, 1)
struct IDX
{
    int ZoneId;
    unsigned int FileSizeBytes;
    unsigned int Crc32;
    char ZoneName[64];
};
#pragma pack(pop)

class CBinZoneIdx :
    public CBinFile
{
private:
    std::vector<IDX> m_vecZoneIdx;

    virtual bool ChunkToData() override;
public:
	const std::vector<IDX>& GetZoneIdxVector() const { return m_vecZoneIdx; }
};

