#pragma once
#include <atomic>

class CObject
{
public:
	CObject();

private:
	std::atomic<int> m_iRef;

	void AddRef() { m_iRef.fetch_add(1); }
	void ReleaseRef() { m_iRef.fetch_sub(1); }
	virtual void OnFree() = 0;
protected:
	void InitRef() { m_iRef.store(1); }
	void AddRef(std::atomic<int>& ref);
	void ReleaseRef(std::atomic<int>& ref, int debugtype = 0);

public:
	int GetRef() { return m_iRef.load(); }
	
};