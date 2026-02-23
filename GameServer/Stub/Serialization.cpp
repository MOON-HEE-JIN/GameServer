#include "Serialization.h" 
#include <memory.h>

int Serialization (char* buffer, st_CTS_ChangePid& value)
{
	int iSize = 0;
	memcpy(buffer + iSize, &value.pid, sizeof(value.pid));
	iSize += sizeof(value.pid);
	return iSize;
}

int Serialization (char* buffer, st_CTS_LoopBack& value)
{
	int iSize = 0;
	memcpy(buffer + iSize, &value.zone, sizeof(value.zone));
	iSize += sizeof(value.zone);
	memcpy(buffer + iSize, &value.data, sizeof(value.data));
	iSize += sizeof(value.data);
	return iSize;
}

int Serialization (char* buffer, st_Header& value)
{
	int iSize = 0;
	memcpy(buffer + iSize, &value.type, sizeof(value.type));
	iSize += sizeof(value.type);
	memcpy(buffer + iSize, &value.size, sizeof(value.size));
	iSize += sizeof(value.size);
	return iSize;
}

int Serialization (char* buffer, st_STC_ChangePid& value)
{
	int iSize = 0;
	memcpy(buffer + iSize, &value.ret, sizeof(value.ret));
	iSize += sizeof(value.ret);
	return iSize;
}

int Serialization (char* buffer, st_STC_LoopBack& value)
{
	int iSize = 0;
	memcpy(buffer + iSize, &value.ret, sizeof(value.ret));
	iSize += sizeof(value.ret);
	memcpy(buffer + iSize, &value.zone, sizeof(value.zone));
	iSize += sizeof(value.zone);
	memcpy(buffer + iSize, &value.data, sizeof(value.data));
	iSize += sizeof(value.data);
	return iSize;
}

int UnSerialization (char* buffer, st_CTS_ChangePid& value)
{
	int iSize = 0;
	memcpy(&value.pid, buffer + iSize, sizeof(value.pid));
	iSize += sizeof(value.pid);
	return iSize;
}

int UnSerialization (char* buffer, st_CTS_LoopBack& value)
{
	int iSize = 0;
	memcpy(&value.zone, buffer + iSize, sizeof(value.zone));
	iSize += sizeof(value.zone);
	memcpy(&value.data, buffer + iSize, sizeof(value.data));
	iSize += sizeof(value.data);
	return iSize;
}

int UnSerialization (char* buffer, st_Header& value)
{
	int iSize = 0;
	memcpy(&value.type, buffer + iSize, sizeof(value.type));
	iSize += sizeof(value.type);
	memcpy(&value.size, buffer + iSize, sizeof(value.size));
	iSize += sizeof(value.size);
	return iSize;
}

int UnSerialization (char* buffer, st_STC_ChangePid& value)
{
	int iSize = 0;
	memcpy(&value.ret, buffer + iSize, sizeof(value.ret));
	iSize += sizeof(value.ret);
	return iSize;
}

int UnSerialization (char* buffer, st_STC_LoopBack& value)
{
	int iSize = 0;
	memcpy(&value.ret, buffer + iSize, sizeof(value.ret));
	iSize += sizeof(value.ret);
	memcpy(&value.zone, buffer + iSize, sizeof(value.zone));
	iSize += sizeof(value.zone);
	memcpy(&value.data, buffer + iSize, sizeof(value.data));
	iSize += sizeof(value.data);
	return iSize;
}

