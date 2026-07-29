#pragma once
#include "../CEntity.h"
#include "../GameServerDef.h"

#include <vector>
#include <unordered_map>

class CEntityVector
{
public:
	CEntityVector() {};
	~CEntityVector() {};

private:
	std::vector<CEntity*> m_vec;
	std::unordered_map<CEntity*, int> m_map;

public:
	const std::vector<CEntity*>& GetVector() { return m_vec; }

	int GetSize() { return static_cast<int>(m_vec.size()); }

	bool AddEntity(CEntity* pEntity)
	{
		if (pEntity == nullptr || m_map.find(pEntity) != m_map.end())
			return false;

		int index = static_cast<int>(m_vec.size());

		m_vec.push_back(pEntity);
		m_map.emplace(pEntity, index);

		pEntity->AddMagRef();
		return true;
	}
	bool RemoveEntity(CEntity* pEntity)
	{
		if (m_vec.empty())
			return false;

		auto it = m_map.find(pEntity);
		if (it == m_map.end())
			return false;

		int index = it->second;
		
		if (index < 0 || index >= static_cast<int>(m_vec.size()) || m_vec[index] != pEntity)
			return false;

		int lastIndex = static_cast<int>(m_vec.size()) - 1;

		if (index != lastIndex)
		{
			CEntity* pEnd = m_vec[lastIndex];

			m_vec[index] = pEnd;
			m_map[pEnd] = index;
		}

		m_vec.pop_back();
		m_map.erase(it);

		// 컨테이너 상태를 먼저 정리한 뒤 소유 참조를 반환한다.
		pEntity->ReleaseMagRef();

		return true;
	}
};
