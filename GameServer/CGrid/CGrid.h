#pragma once
#include "../Stub/ProjectDefineStruct.h"
#include "CGridDefines.h"
#include <vector>

class CGrid
{
public:
	CGrid();
	~CGrid();

protected:
	int m_iID;
	int m_iSparseBlockSize;
	float m_fVoxelSize;

	st_Vector3F m_stOrigin;
	st_Vector3F m_stMinExtent;
	st_Vector3F m_stMaxExtent;

	st_Vector3D m_stGridSize;
	
	std::vector<std::vector<unsigned __int8>> m_vecBitData;

public:
	
	void PushBitData(eBitType type, const std::vector<unsigned __int8>& data)
	{
		if (type < m_vecBitData.size())
		{
			m_vecBitData[type] = data;
		}
	}
private:
	__int64 GetBitIndex(const st_Vector3D& position);

public:
	int GetGridID() { return m_iID; }
	st_Vector3F GetOrigin() { return m_stOrigin; }
	st_Vector3F GetMinExtent() { return m_stMinExtent; }
	st_Vector3F GetMaxExtent() { return m_stMaxExtent; }
	st_Vector3D GetGridSize() { return m_stGridSize; }

	st_Vector3D WorldToGrid(const st_Vector3F& pos);
	st_Vector3F GridToWorld(const st_Vector3D& gridPos);

	bool GetBit(eBitType type, const st_Vector3F& position);
	bool GetBit(eBitType type, const st_Vector3D& position);

	friend class CBinFileManager;
};