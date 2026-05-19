#pragma once

#include <vector>
#include <unordered_map>
#include <stack>
#include <atomic>

template <class T>
class CTContainer
{
public:
	CTContainer() = default;
	~CTContainer() = default;

private:
	std::vector<T*> vec;
	std::unordered_map<int, int> keys;
	std::stack<int> m_stack;
	std::atomic<int> size;
public:
	bool Add(int key, T* value);
	bool Sub(int key, T* value);
	int GetSize() { return size.load(); }
};

template<class T>
inline bool CTContainer<T>::Add(int key, T* value)
{
	if (keys.find(key) != keys.end())
		return false;
	bool b = m_stack.empty();
	if (m_stack.empty())
	{
		keys[key] = static_cast<int>(vec.size());
		vec.push_back(value);
		size.fetch_add(1);
		return true;
	}

	int index = m_stack.top();
	m_stack.pop();

	vec[index] = value;
	keys[key] = index;
	size.fetch_add(1);
	return true;
}

template<class T>
inline bool CTContainer<T>::Sub(int key, T* value)
{
	std::unordered_map<int, int>::iterator iter = keys.find(key);

	if (iter == keys.end())
		return false;

	int subindex = iter->second;

	if (vec[subindex] != value)
		return false;

	vec[subindex] = nullptr;
	keys.erase(iter);
	
	m_stack.push(subindex);
	size.fetch_sub(1);
	return true;
}
