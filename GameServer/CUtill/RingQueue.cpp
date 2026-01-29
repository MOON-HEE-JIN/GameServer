#include "RingQueue.h"

#include<malloc.h>
#include <memory.h>

#define DEFAULTSIZE 5000



RingQueue::RingQueue()
{
	buffer = (char*)malloc(sizeof(char) * DEFAULTSIZE);
	ReadPointer = buffer;
	WritePointer = buffer;
	FirstBufferPoint = buffer;
	EndBufferPoint = buffer + DEFAULTSIZE - 1;
	PacketBufferPointer = buffer;

}

RingQueue::RingQueue(int size)
{
	buffer = (char*)malloc(sizeof(char) * (size));
	ReadPointer = buffer;
	WritePointer = buffer;
	FirstBufferPoint = buffer;
	EndBufferPoint = buffer + size - 1;
	PacketBufferPointer = buffer;
}

RingQueue::~RingQueue()
{
	free(buffer);
}

int RingQueue::GetUseSize()
{
	if(WritePointer >= ReadPointer)
		return WritePointer - ReadPointer;
	return (WritePointer - FirstBufferPoint) + (EndBufferPoint - ReadPointer);
}

int RingQueue::GetFreeSize()
{
	if (WritePointer >= ReadPointer)
		return (ReadPointer - FirstBufferPoint) + (EndBufferPoint - WritePointer);
	return ReadPointer - WritePointer - 1;
}

int RingQueue::GetDirectEnqueueSize()
{
	if (WritePointer >= ReadPointer)
		return EndBufferPoint - WritePointer;
	return ReadPointer - WritePointer - 1;
}

int RingQueue::GetDirectDequeueSize()
{
	if (WritePointer >= ReadPointer)
		return WritePointer - ReadPointer;
	return (EndBufferPoint - ReadPointer);
	
}

int RingQueue::MoveWritePointer(int size)
{
	WritePointer += size;
	if (WritePointer >= EndBufferPoint)
	{
		int OverPointer = WritePointer - EndBufferPoint;
		WritePointer = FirstBufferPoint + OverPointer;
	}
	return 0;
}

int RingQueue::MoveReadPointer(int size)
{
	ReadPointer += size;
	if (ReadPointer >= EndBufferPoint)
	{
		int OverPointer = ReadPointer - EndBufferPoint;
		ReadPointer = FirstBufferPoint + OverPointer;
	}
	return 0;
}
int RingQueue::MovePacketPointer(int size)
{
	PacketBufferPointer += size;
	if (PacketBufferPointer >= EndBufferPoint)
	{
		int OverPointer = PacketBufferPointer - EndBufferPoint;
		PacketBufferPointer = FirstBufferPoint + OverPointer;
	}
	
	return 0;
}
int RingQueue::Enqueue(const char* buf, int size)
{
	if (GetFreeSize() < size) return 0;
	//크기가 충분할때
	if (GetDirectEnqueueSize() >= size)
	{
		memcpy_s(WritePointer, size, buf, size);
		MoveWritePointer(size);
		return size;
	}
	
	const char* CopyPointer = buf;
	int EnqueueSize = GetDirectEnqueueSize();
	memcpy_s(WritePointer, EnqueueSize, CopyPointer, EnqueueSize);
	CopyPointer += EnqueueSize;
	MoveWritePointer(EnqueueSize);

	int RemzinSize = size - EnqueueSize;
	memcpy_s(WritePointer, RemzinSize, CopyPointer, RemzinSize);
	MoveWritePointer(RemzinSize);

	return size;
}

int RingQueue::Dequeue(char* buf, int size)
{
	if (GetUseSize() < size) return 0;
	//크기가 충분할때
	if (GetDirectDequeueSize() >= size)
	{
		memcpy_s(buf, size, ReadPointer, size);
		MoveReadPointer(size);
		return size;
	}

	char* CopyPointer = buf;
	int DequeueSize = GetDirectDequeueSize();
	memcpy_s(CopyPointer, DequeueSize, ReadPointer, DequeueSize);
	CopyPointer += DequeueSize;
	MoveReadPointer(DequeueSize);

	int RemzinSize = size - DequeueSize;
	memcpy_s(CopyPointer, RemzinSize, ReadPointer, RemzinSize);
	MoveReadPointer(RemzinSize);
	/*
	MPacket* mPacket = (MPacket*)buf;
	mPacket->Len;
	*/
	return size;
}

int RingQueue::Peek(char* buf, int size)
{
	if (GetUseSize() < size) return 0;
	//크기가 충분할때
	if (GetDirectDequeueSize() >= size)
	{
		memcpy_s(buf, size, ReadPointer, size);
		//MoveReadPointer(size);
		return size;
	}
	char* origniPointer = ReadPointer;
	char* CopyPointer = buf;
	int DequeueSize = GetDirectDequeueSize();
	memcpy_s(CopyPointer, DequeueSize, ReadPointer, DequeueSize);
	CopyPointer += DequeueSize;
	MoveReadPointer(DequeueSize);

	int RemzinSize = size - DequeueSize;
	memcpy_s(CopyPointer, RemzinSize, ReadPointer, RemzinSize);
	ReadPointer = origniPointer;
	return size;
} 


