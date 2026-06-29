#include "CRingBuffer.h"

CRingBuffer::CRingBuffer()
{
	m_pBuffer = new char[DEFAULT_BUFFER_SIZE];

	m_iUseSize = 0;
	m_iFreeSize = DEFAULT_BUFFER_SIZE;

	m_pReadPointer = m_pBuffer;
	m_pWritePointer = m_pBuffer;
	m_pEndPointer = m_pBuffer + DEFAULT_BUFFER_SIZE;
}

CRingBuffer::~CRingBuffer()
{
	delete[] m_pBuffer;
}

int CRingBuffer::GetDirectEnqueueSize()
{
	if (m_iFreeSize.load() == 0)
		return 0;

	__int64 iDirectEnqueueSize = 0;
	
	if (m_pReadPointer > m_pWritePointer)
		iDirectEnqueueSize = m_pReadPointer - m_pWritePointer;
	else
		iDirectEnqueueSize = m_pEndPointer - m_pWritePointer;

	return static_cast<int>(iDirectEnqueueSize);
}

int CRingBuffer::GetDirectDequeueSize()
{
	if (m_iUseSize.load() == 0)
		return 0;

	__int64 iDirectDequeueSize = 0;

	if (m_pWritePointer > m_pReadPointer)
		iDirectDequeueSize = m_pWritePointer - m_pReadPointer;
	else
		iDirectDequeueSize = m_pEndPointer - m_pReadPointer;

	return static_cast<int>(iDirectDequeueSize);
}

void CRingBuffer::PrivateMoveReadPointer(int size)
{
	m_pReadPointer += size;
	if (m_pReadPointer >= m_pEndPointer)
	{
		__int64 iOverPointer = m_pReadPointer - m_pEndPointer;
		m_pReadPointer = m_pBuffer + iOverPointer;
	}
}

void CRingBuffer::PrivateMoveWritePointer(int size)
{
	m_pWritePointer += size;
	if (m_pWritePointer >= m_pEndPointer)
	{
		__int64 iOverPointer = m_pWritePointer - m_pEndPointer;
		m_pWritePointer = m_pBuffer + iOverPointer;
	}
}

void CRingBuffer::MoveReadPointer(int size)
{
	m_pReadPointer += size;
	if (m_pReadPointer >= m_pEndPointer)
	{
		__int64 iOverPointer = m_pReadPointer - m_pEndPointer;
		m_pReadPointer = m_pBuffer + iOverPointer;
	}
	m_iUseSize.fetch_sub(size);
	m_iFreeSize.fetch_add(size);
}

void CRingBuffer::MoveWritePointer(int size)
{
	m_pWritePointer += size;
	if (m_pWritePointer >= m_pEndPointer)
	{
		__int64 iOverPointer = m_pWritePointer - m_pEndPointer;
		m_pWritePointer = m_pBuffer + iOverPointer;
	}
	m_iUseSize.fetch_add(size);
	m_iFreeSize.fetch_sub(size);
}

int CRingBuffer::Peek(char* buf, int size)
{
	if (GetUseSize() < size)
		return 0;

	if (GetDirectDequeueSize() >= size)
	{
		memcpy(buf, m_pReadPointer, size);
	}
	else
	{
		int iFirstPeekSize = GetDirectDequeueSize();
		memcpy(buf, m_pReadPointer, iFirstPeekSize);

		int iSecondPeekSize = size - iFirstPeekSize;
		memcpy(buf + iFirstPeekSize, m_pBuffer, iSecondPeekSize);
	}
	return size;
}

int CRingBuffer::Enqueue(const char* buf, int size)
{
	if (GetFreeSize() < size)
		return 0;

	if (GetDirectEnqueueSize() >= size)
	{
		memcpy(m_pWritePointer, buf, size);
		PrivateMoveWritePointer(size);
	}
	else
	{
		int iFirstEnqueueSize = GetDirectEnqueueSize();
		memcpy(m_pWritePointer, buf, iFirstEnqueueSize);
		PrivateMoveWritePointer(iFirstEnqueueSize);

		int iSecondEnqueueSize = size - iFirstEnqueueSize;
		memcpy(m_pWritePointer, buf + iFirstEnqueueSize, iSecondEnqueueSize);
		PrivateMoveWritePointer(iSecondEnqueueSize);
	}

	m_iUseSize.fetch_add(size);
	m_iFreeSize.fetch_sub(size);
	return size;
}

int CRingBuffer::Dequeue(char* buf, int size)
{
	if (GetUseSize() < size)
		return 0;

	if (GetDirectDequeueSize() >= size)
	{
		memcpy(buf, m_pReadPointer, size);
		PrivateMoveReadPointer(size);
	}
	else
	{
		int iFirstDequeueSize = GetDirectDequeueSize();
		memcpy(buf, m_pReadPointer, iFirstDequeueSize);
		PrivateMoveReadPointer(iFirstDequeueSize);

		int iSecondDequeueSize = size - iFirstDequeueSize;
		memcpy(buf + iFirstDequeueSize, m_pReadPointer, iSecondDequeueSize);
		PrivateMoveReadPointer(iSecondDequeueSize);
	}

	m_iUseSize.fetch_sub(size);
	m_iFreeSize.fetch_add(size);
	return size;
}

void CRingBuffer::Clear()
{
	m_iUseSize.store(0);
	m_iFreeSize.store(DEFAULT_BUFFER_SIZE);
	m_pReadPointer = m_pBuffer;
	m_pWritePointer = m_pBuffer;
}
