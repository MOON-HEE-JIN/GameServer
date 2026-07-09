#pragma once
#include "../CEntity.h"
#include "../GameServerDef.h"

#include <vector>
#include <map>

class CEntityVector
{
public:
	CEntityVector(EIndexType type) { m_iKeyType = (int)type; m_iCount = 0; };
	~CEntityVector() {};

private:
	int m_iKeyType;
	int m_iCount;
	std::vector<CEntity*> m_vec;
public:
	const std::vector<CEntity*>& GetVector() { return m_vec; }

	int GetSize() { return m_iCount; }

	bool AddEntity(CEntity* pEntity)
	{
		if (pEntity->GetVectorIndex(m_iKeyType) != -1)
			return false;

		int index = static_cast<int>(m_vec.size());
		if (!pEntity->SetVectorIndex(m_iKeyType, index))
			return false;

		m_vec.push_back(pEntity);
		m_iCount++;
		return true;
	}
	bool RemoveEntity(CEntity* pEntity)
	{
		if (m_vec.empty())
			return false;

		int index = pEntity->GetVectorIndex(m_iKeyType);
		
		if (index == -1)
			return false;

		if (m_vec[index] != pEntity)
			return false;

		pEntity->ResetVectorIndex(m_iKeyType);
		int lastIndex = static_cast<int>(m_vec.size()) - 1;

		if (index != lastIndex)
		{
			CEntity* pEnd = m_vec[lastIndex];

			m_vec[index] = pEnd;
			pEnd->SetVectorIndex(m_iKeyType, index);
		}

		m_iCount--;
		m_vec.pop_back();
		return true;
	}
};
