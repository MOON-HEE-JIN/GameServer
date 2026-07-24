#pragma once
#include "../CEntity.h"
#include "../GameServerDef.h"

#include <vector>
#include <map>

class CEntityVector
{
public:
	CEntityVector() {};
	~CEntityVector() {};

private:
	std::vector<CEntity*> m_vec;
	std::map<CEntity*, int> m_map;

public:
	const std::vector<CEntity*>& GetVector() { return m_vec; }

	int GetCount() { return static_cast<int>(m_vec.size()); }

	bool AddEntity(CEntity* pEntity)
	{
		int index = static_cast<int>(m_vec.size());

		m_vec.push_back(pEntity);
		m_map[pEntity] = index;

		pEntity->AddMagRef();
		return true;
	}
	bool RemoveEntity(CEntity* pEntity)
	{
		if (m_vec.empty())
			return false;

		if (m_map.find(pEntity) == m_map.end())
			return false;

		int index = m_map[pEntity];
		
		if (m_vec[index] != pEntity)
			return false;

		int lastIndex = static_cast<int>(m_vec.size()) - 1;

		if (index != lastIndex)
		{
			CEntity* pEnd = m_vec[lastIndex];

			m_vec[index] = pEnd;
			m_map[pEnd] = index;
		}

		pEntity->ReleaseMagRef();
		
		m_vec.pop_back();
		m_map.erase(pEntity);

		return true;
	}
};
