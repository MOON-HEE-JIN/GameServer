#pragma once
#include "../Stub/Serialization.h"

class CPacket
{
public:
	CPacket();
	CPacket(int size);
	CPacket(const CPacket& other);
	virtual ~CPacket();

	void Release();//패킷 파괴
	virtual void Clear();//패킷 청소

	int GetBufferSize() { return m_iBufferSize; }
	int GetDataSize() { return (int)(WritePointer - ReadPointer); }

	char* GetBufferPtr() { return buffer; }

	char* GetReadBuffPtr() { return ReadPointer; }
	char* GetWriteBuffPtr() { return WritePointer; }

	int MoveWritePos(int size);
	int MoveReadPos(int size);

	int GetData(void* Dest, int size);
	int PutData(void* Src, int size);
protected:
	char* buffer;
	char* WritePointer;
	char* ReadPointer;

	int m_iBufferSize;
private:
	char* EndPointer;

public:
	//연산자 오버로딩
	CPacket& operator = (const CPacket& clSrcPacket);
	//INPUT
	CPacket& operator << (unsigned char value);
	CPacket& operator << (char value);
	CPacket& operator << (unsigned short value);
	CPacket& operator << (short value);
	CPacket& operator << (unsigned int value);
	CPacket& operator << (int value);
	CPacket& operator << (long value);
	CPacket& operator << (float value);
	CPacket& operator << (double value);
	CPacket& operator << (__int64 value);
	CPacket& operator << (bool value);
	template<typename T>
	CPacket& operator << (T value);

	//OUTPUT
	CPacket& operator >> (unsigned char& value);
	CPacket& operator >> (char& value);
	CPacket& operator >> (unsigned short& value);
	CPacket& operator >> (short& value);
	CPacket& operator >> (unsigned int& value);
	CPacket& operator >> (int& value);
	CPacket& operator >> (long& value);
	CPacket& operator >> (float& value);
	CPacket& operator >> (double& value);
	CPacket& operator >> (__int64& value);
	CPacket& operator >> (bool& value);
	template<typename T>
	CPacket& operator >> (T& value);
};

template<typename T>
inline CPacket& CPacket::operator<<(T value)
{
	// TODO: 여기에 return 문을 삽입합니다.
	int len = Serialization(GetWriteBuffPtr(), value);
	MoveWritePos(len);
	return *this;
}

template<typename T>
inline CPacket& CPacket::operator>>(T& value)
{
	// TODO: 여기에 return 문을 삽입합니다.
	int len = UnSerialization(GetReadBuffPtr(), value);
	MoveReadPos(len);
	return *this;
}


