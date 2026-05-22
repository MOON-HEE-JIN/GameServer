#pragma once
#include "../CEntity.h"
#include "../GameServerDef.h"

#include <vector>
#include <map>

class CEntityVector
{
public:
	CEntityVector(EVECTOR_INDEX_TYPE::EIndexType type) { m_iKeyType = (int)type; };
	~CEntityVector() {};

private:
	int m_iKeyType;
	std::vector<CEntity*> m_vec;
public:
	const std::vector<CEntity*>& GetVector() { return m_vec; }

	bool AddEntity(CEntity* pEntity)
	{
		int index = static_cast<int>(m_vec.size());
		if (!pEntity->SetVectorIndex(m_iKeyType, index))
			return false;

		m_vec.push_back(pEntity);

		return true;
	}
	bool RemoveEntity(CEntity* pEntity)
	{
		int index = pEntity->GetVectorIndex(m_iKeyType);
		if (index == -1)
			return false;
		pEntity->SetVectorIndex(m_iKeyType, -1);
		int lastIndex = static_cast<int>(m_vec.size()) - 1;

		if (index != lastIndex)
		{
			CEntity* pEnd = m_vec[lastIndex];

			m_vec[index] = pEnd;
			pEnd->SetVectorIndex(m_iKeyType, index);
		}

		m_vec.pop_back();
		return true;
	}
};
