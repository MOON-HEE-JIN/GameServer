#pragma once
#include "../CEntity.h"

#include <vector>
#include <map>

class CEntityManagementVector
{
public:
	CEntityManagementVector() {};
	~CEntityManagementVector() {};

private:
	std::vector<CEntity*> m_vec;
public:
	const std::vector<CEntity*>& GetVector() { return m_vec; }

	void AddEntity(CEntity* pEntity)
	{
		pEntity->SetMoveIndex(m_vec.size());
		m_vec.push_back(pEntity);
	}
	void RemoveEntity(CEntity* pEntity)
	{
		int index = pEntity->GetMoveIndex();

		int lastIndex = m_vec.size() - 1;

		if (index != lastIndex)
		{
			CEntity* pEnd = m_vec[lastIndex];

			m_vec[index] = pEnd;
			pEnd->SetMoveIndex(index);
		}

		m_vec.pop_back();
	}
};
