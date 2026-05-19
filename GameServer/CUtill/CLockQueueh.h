#pragma once
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <queue>

template<typename T>
class CLQueue
{
public:
	CLQueue();
	~CLQueue();

private:
	std::queue<T> m_queue;
	CRITICAL_SECTION cs;

public:
	void Push(T value);
	bool POP(T& out);
};

template<typename T>
inline CLQueue<T>::CLQueue()
{
	InitializeCriticalSection(&cs);
}

template<typename T>
inline CLQueue<T>::~CLQueue()
{
	DeleteCriticalSection(&cs);
}

template<typename T>
inline void CLQueue<T>::Push(T value)
{
	EnterCriticalSection(&cs);
	m_queue.push(value);
	LeaveCriticalSection(&cs);
}

template<typename T>
inline bool CLQueue<T>::POP(T& out)
{
	EnterCriticalSection(&cs);
	if (m_queue.empty())
	{
		LeaveCriticalSection(&cs);
		return false;
	}
	out = m_queue.front();
	m_queue.pop();
	LeaveCriticalSection(&cs);
	return true;
}
