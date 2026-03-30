#include "CPacket.h"
#include <malloc.h>
#include <memory.h>
#include <stdio.h>

static const int DefualtSize = 4096;

struct PACKET_BUFFER
{
	char buffer[DefualtSize];
};


CPacket::CPacket()
{
	buffer = (char*)malloc(DefualtSize);
	WritePointer = buffer;
	ReadPointer = buffer;
	m_iBufferSize = DefualtSize;

	EndPointer = buffer + DefualtSize;
}

CPacket::CPacket(int size)
{
	//pool.Free((PACKET_BUFFER*)buffer);
	buffer = (char*)malloc(size);
	WritePointer = buffer;
	ReadPointer = buffer;
	m_iBufferSize = size;

	EndPointer = buffer + size;
}

CPacket::~CPacket()
{
	free(buffer);
}

void CPacket::Release()
{
}

void CPacket::Clear()
{
	WritePointer = buffer;
	ReadPointer = buffer;
}

int CPacket::MoveWritePos(int size)
{
	if ((WritePointer + size) > EndPointer)
		return 0;
	WritePointer += size;
	return size;
}

int CPacket::MoveReadPos(int size)
{
	if ((ReadPointer + size) > WritePointer)
		return 0;
	ReadPointer += size;
	return size;
}

int CPacket::GetData(void* Dest, int size)
{
	memcpy(Dest, ReadPointer, size);
	MoveReadPos(size);
	return size;
}

int CPacket::PutData(void* Src, int size)
{
	if (size > m_iBufferSize - GetDataSize())
		return 0;
	memcpy(WritePointer, Src, size);
	MoveWritePos(size);
	return size;
}


CPacket& CPacket::operator<<(unsigned char value)
{
	// TODO: 여기에 return 문을 삽입합니다.
	int size = PutData((char*)&value, sizeof(unsigned char));
	return *this;
}

CPacket& CPacket::operator<<(char value)
{
	// TODO: 여기에 return 문을 삽입합니다.
	int size = PutData((char*)&value, sizeof(char));
	return *this;
}

CPacket& CPacket::operator<<(unsigned short value)
{
	// TODO: 여기에 return 문을 삽입합니다.
	int size = PutData((char*)&value, sizeof(unsigned short));
	return *this;
}

CPacket& CPacket::operator<<(short value)
{
	// TODO: 여기에 return 문을 삽입합니다.
	int size = PutData((char*)&value, sizeof(short));
	return *this;
}

CPacket& CPacket::operator<<(unsigned int value)
{
	// TODO: 여기에 return 문을 삽입합니다.
	int size = PutData((char*)&value, sizeof(unsigned int));
	return *this;
}

CPacket& CPacket::operator<<(int value)
{
	// TODO: 여기에 return 문을 삽입합니다.
	int size = PutData((char*)&value, sizeof(int));
	return *this;
}

CPacket& CPacket::operator<<(long value)
{
	// TODO: 여기에 return 문을 삽입합니다.
	int size = PutData((char*)&value, sizeof(long));
	return *this;
}

CPacket& CPacket::operator<<(float value)
{
	// TODO: 여기에 return 문을 삽입합니다.
	int size = PutData((char*)&value, sizeof(float));
	return *this;
}

CPacket& CPacket::operator<<(double value)
{
	// TODO: 여기에 return 문을 삽입합니다.
	int size = PutData((char*)&value, sizeof(double));
	return *this;
}

CPacket& CPacket::operator<<(__int64 value)
{
	// TODO: 여기에 return 문을 삽입합니다.
	int size = PutData((char*)&value, sizeof(__int64));
	return *this;
}

CPacket& CPacket::operator<<(bool value)
{
	// TODO: 여기에 return 문을 삽입합니다.
	int size = PutData((char*)&value, sizeof(bool));
	return *this;
}

//OUTPUT

CPacket& CPacket::operator>>(unsigned char& value)
{
	// TODO: 여기에 return 문을 삽입합니다.
	int size = GetData((char*)&value, sizeof(unsigned char));
	return *this;
}

CPacket& CPacket::operator>>(char& value)
{
	//TODO: 여기에 return 문을 삽입합니다.
	int size = GetData((char*)&value, sizeof(char));
	return *this;
}

CPacket& CPacket::operator>>(unsigned short& value)
{
	// TODO: 여기에 return 문을 삽입합니다.
	int size = GetData((char*)&value, sizeof(short));
	return *this;
}

CPacket& CPacket::operator>>(short& value)
{
	// TODO: 여기에 return 문을 삽입합니다.
	int size = GetData((char*)&value, sizeof(short));
	return *this;
}

CPacket& CPacket::operator>>(unsigned int& value)
{
	// TODO: 여기에 return 문을 삽입합니다.
	int size = GetData((char*)&value, sizeof(unsigned int));
	return *this;
}

CPacket& CPacket::operator>>(int& value)
{
	// TODO: 여기에 return 문을 삽입합니다.
	int size = GetData((char*)&value, sizeof(int));
	return *this;
}

CPacket& CPacket::operator>>(long& value)
{
	// TODO: 여기에 return 문을 삽입합니다.
	int size = GetData((char*)&value, sizeof(long));
	return *this;
}

CPacket& CPacket::operator>>(float& value)
{
	// TODO: 여기에 return 문을 삽입합니다.
	int size = GetData((char*)&value, sizeof(float));
	return *this;
}

CPacket& CPacket::operator>>(double& value)
{
	// TODO: 여기에 return 문을 삽입합니다.
	int size = GetData((char*)&value, sizeof(double));
	return *this;
}

CPacket& CPacket::operator>>(__int64& value)
{
	// TODO: 여기에 return 문을 삽입합니다.
	int size = GetData((char*)&value, sizeof(__int64));
	return *this;
}

CPacket& CPacket::operator>>(bool& value)
{
	// TODO: 여기에 return 문을 삽입합니다.
	int size = GetData((char*)&value, sizeof(bool));
	return *this;
}

// 복사 생성자
CPacket::CPacket(const CPacket& other)
{
	m_iBufferSize = other.m_iBufferSize;
	buffer = (char*)malloc(m_iBufferSize);
	memcpy(buffer, other.buffer, m_iBufferSize);
	EndPointer = buffer + m_iBufferSize;
	ReadPointer = buffer + (other.ReadPointer - other.buffer);
	WritePointer = buffer + (other.WritePointer - other.buffer);
}

// 안전한 복사 대입 연산자
CPacket& CPacket::operator=(const CPacket& other)
{
	if (this == &other) return *this;

	if (m_iBufferSize < other.m_iBufferSize)
	{
		free(buffer);
		m_iBufferSize = other.m_iBufferSize;
		buffer = (char*)malloc(m_iBufferSize);
	}
	memcpy(buffer, other.buffer, other.m_iBufferSize);
	EndPointer = buffer + m_iBufferSize;
	ReadPointer = buffer + (other.ReadPointer - other.buffer);
	WritePointer = buffer + (other.WritePointer - other.buffer);
	return *this;
}
