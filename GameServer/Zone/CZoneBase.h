#pragma once

#include "../Stub/ProjectDefineStruct.h"
#include "../GameServerDef.h"
#include <atomic>
class CZoneBase
{
public:
	CZoneBase(int channel, int ZoneID, int ProcID, int Maximum);
	~CZoneBase();
	
private:
	int m_iChannel;
	int m_iZoneID;
	int m_iProcID;
	int m_iMaximumUser;

	std::atomic<int> m_iCount;
protected:
	bool m_bMainWorld = false;
	std::atomic<bool> m_bActive;
	
	int m_iWidth;
	int m_iHeight;

public:
	virtual void Reset();

	virtual void Process() = 0;
public:
	int GetChannel() { return m_iChannel; }
	int GetZoneID() { return m_iZoneID; }
	int GetProcID() { return m_iProcID; }
	int GetMaximum() { return m_iMaximumUser; }
	int GetCurCnt() { return m_iCount.load(); }
	int GetWidth() { return m_iWidth; }
	int GetHeight() { return m_iHeight; }
	bool GetMainWorld() { return m_bMainWorld; }

	void AddCount() { m_iCount.fetch_add(1); }
	void SubCount() { m_iCount.fetch_sub(1); }
public:
	bool CheckPos(st_Vector3F pos);
};