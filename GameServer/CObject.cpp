#include "CObject.h"

#include "Log/CLog.h"

CObject::CObject()
{
	m_iRef.store(1);
}

void CObject::AddRef(std::atomic<int>& ref)
{
	ref.fetch_add(1);
	AddRef();
}

void CObject::ReleaseRef(std::atomic<int>& ref, int debugtype)
{
	ref.fetch_sub(1);
	if (ref.load() < 0)
	{
		g_LogRef.ELog("Ref < 0 Ref Type : %d", debugtype);
		return;
	}

	m_iRef.fetch_sub(1);
	if (m_iRef.load() < 0)
	{
		g_LogRef.ELog("m_iRef < 0 Type : %d", debugtype);
	}

	if (m_iRef.load() > 0)
		return;

	OnFree();
}
