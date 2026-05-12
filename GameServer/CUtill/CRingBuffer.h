#pragma once

#define DEFAULT_BUFFER_SIZE 10000

#include <atomic>

class CRingBuffer
{
public:
	CRingBuffer();
	~CRingBuffer();

private:
	char* m_pBuffer;
	std::atomic<int> m_iUseSize;
	std::atomic<int> m_iFreeSize;

	char* m_pReadPointer;
	char* m_pWritePointer;
	char* m_pEndPointer;

public:
	int GetUseSize() { return m_iUseSize.load(); };
	int GetFreeSize() { return m_iFreeSize.load(); };
	int GetDirectEnqueueSize();
	int GetDirectDequeueSize();

	const void* GetReadPointer() { return m_pReadPointer; }
	const void* GetWritePointer() { return m_pWritePointer; }
	const void* GetBuffer() { return m_pBuffer; }
	const void* GetEndPointer() { return m_pEndPointer; }

private:
	void PrivateMoveReadPointer(int size);
	void PrivateMoveWritePointer(int size);

public:
	void MoveReadPointer(int size);
	void MoveWritePointer(int size);

	int Peek(char* buf, int size);
	int Enqueue(const char* buf, int size);
	int Dequeue(char* buf, int size);

	void Clear();
};

