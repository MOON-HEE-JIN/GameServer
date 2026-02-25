#include "Serialization.h" 
#include "PacketEnumDef.h"
#include <memory.h>

int Serialization (char* buffer, st_CTS_ChangeZone& value)
{
	int hSize = 0;
	st_Header header;
	int iSize = sizeof(st_Header);
	memcpy(buffer + iSize, &value.zone, sizeof(value.zone));
	iSize += sizeof(value.zone);

	header.type = GAME::CHANGEZONE;
	header.size = iSize - sizeof(st_Header);
	Serialization(buffer, header);
	return iSize;
}

int Serialization (char* buffer, st_CTS_EnterZone& value)
{
	int hSize = 0;
	st_Header header;
	int iSize = sizeof(st_Header);
	memcpy(buffer + iSize, &value.zone, sizeof(value.zone));
	iSize += sizeof(value.zone);

	header.type = GAME::ENTERZONE;
	header.size = iSize - sizeof(st_Header);
	Serialization(buffer, header);
	return iSize;
}

int Serialization (char* buffer, st_CTS_LoopBack& value)
{
	int hSize = 0;
	st_Header header;
	int iSize = sizeof(st_Header);
	memcpy(buffer + iSize, &value.zone, sizeof(value.zone));
	iSize += sizeof(value.zone);
	memcpy(buffer + iSize, &value.data, sizeof(value.data));
	iSize += sizeof(value.data);

	header.type = GAME::LOOPBACK;
	header.size = iSize - sizeof(st_Header);
	Serialization(buffer, header);
	return iSize;
}

int Serialization (char* buffer, st_ConnectInfo& value)
{
	int iSize = 0;
	memcpy(buffer + iSize, &value.ID, sizeof(value.ID));
	iSize += sizeof(value.ID);
	return iSize;
}

int Serialization (char* buffer, st_EntityInfo& value)
{
	int iSize = 0;
	memcpy(buffer + iSize, &value.type, sizeof(value.type));
	iSize += sizeof(value.type);
	memcpy(buffer + iSize, &value.ID, sizeof(value.ID));
	iSize += sizeof(value.ID);
	iSize += Serialization(buffer + iSize, value.pos);
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

int Serialization (char* buffer, st_STC_ChangeZone& value)
{
	int hSize = 0;
	st_Header header;
	int iSize = sizeof(st_Header);
	memcpy(buffer + iSize, &value.ret, sizeof(value.ret));
	iSize += sizeof(value.ret);
	memcpy(buffer + iSize, &value.zone, sizeof(value.zone));
	iSize += sizeof(value.zone);

	header.type = GAME::CHANGEZONE;
	header.size = iSize - sizeof(st_Header);
	Serialization(buffer, header);
	return iSize;
}

int Serialization (char* buffer, st_STC_ConnectInfo& value)
{
	int hSize = 0;
	st_Header header;
	int iSize = sizeof(st_Header);
	iSize += Serialization(buffer + iSize, value.info);

	header.type = GAME::CONNECTINFO;
	header.size = iSize - sizeof(st_Header);
	Serialization(buffer, header);
	return iSize;
}

int Serialization (char* buffer, st_STC_CreateChar& value)
{
	int hSize = 0;
	st_Header header;
	int iSize = sizeof(st_Header);
	memcpy(buffer + iSize, &value.ID, sizeof(value.ID));
	iSize += sizeof(value.ID);
	iSize += Serialization(buffer + iSize, value.pos);

	header.type = GAME::CREATECHAR;
	header.size = iSize - sizeof(st_Header);
	Serialization(buffer, header);
	return iSize;
}

int Serialization (char* buffer, st_STC_EnterZone& value)
{
	int hSize = 0;
	st_Header header;
	int iSize = sizeof(st_Header);
	memcpy(buffer + iSize, &value.ret, sizeof(value.ret));
	iSize += sizeof(value.ret);
	memcpy(buffer + iSize, &value.Loop1, sizeof(value.Loop1));
	iSize += sizeof(value.Loop1);
	for(int i = 0; i < value.Loop1; ++i)
	{
		iSize += Serialization(buffer + iSize, value.info[i]);
	}

	header.type = GAME::ENTERZONE;
	header.size = iSize - sizeof(st_Header);
	Serialization(buffer, header);
	return iSize;
}

int Serialization (char* buffer, st_STC_LeaveZone& value)
{
	int hSize = 0;
	st_Header header;
	int iSize = sizeof(st_Header);
	memcpy(buffer + iSize, &value.zone, sizeof(value.zone));
	iSize += sizeof(value.zone);
	memcpy(buffer + iSize, &value.ID, sizeof(value.ID));
	iSize += sizeof(value.ID);

	header.type = GAME::LEAVEZONE;
	header.size = iSize - sizeof(st_Header);
	Serialization(buffer, header);
	return iSize;
}

int Serialization (char* buffer, st_STC_LoopBack& value)
{
	int hSize = 0;
	st_Header header;
	int iSize = sizeof(st_Header);
	memcpy(buffer + iSize, &value.ret, sizeof(value.ret));
	iSize += sizeof(value.ret);
	memcpy(buffer + iSize, &value.zone, sizeof(value.zone));
	iSize += sizeof(value.zone);
	memcpy(buffer + iSize, &value.data, sizeof(value.data));
	iSize += sizeof(value.data);

	header.type = GAME::LOOPBACK;
	header.size = iSize - sizeof(st_Header);
	Serialization(buffer, header);
	return iSize;
}

int Serialization (char* buffer, st_Vector& value)
{
	int iSize = 0;
	memcpy(buffer + iSize, &value.X, sizeof(value.X));
	iSize += sizeof(value.X);
	memcpy(buffer + iSize, &value.Y, sizeof(value.Y));
	iSize += sizeof(value.Y);
	return iSize;
}

int UnSerialization (char* buffer, st_CTS_ChangeZone& value)
{
	int iSize = 0;
	memcpy(&value.zone, buffer + iSize, sizeof(value.zone));
	iSize += sizeof(value.zone);
	return iSize;
}

int UnSerialization (char* buffer, st_CTS_EnterZone& value)
{
	int iSize = 0;
	memcpy(&value.zone, buffer + iSize, sizeof(value.zone));
	iSize += sizeof(value.zone);
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

int UnSerialization (char* buffer, st_ConnectInfo& value)
{
	int iSize = 0;
	memcpy(&value.ID, buffer + iSize, sizeof(value.ID));
	iSize += sizeof(value.ID);
	return iSize;
}

int UnSerialization (char* buffer, st_EntityInfo& value)
{
	int iSize = 0;
	memcpy(&value.type, buffer + iSize, sizeof(value.type));
	iSize += sizeof(value.type);
	memcpy(&value.ID, buffer + iSize, sizeof(value.ID));
	iSize += sizeof(value.ID);
	iSize += UnSerialization(buffer + iSize, value.pos);
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

int UnSerialization (char* buffer, st_STC_ChangeZone& value)
{
	int iSize = 0;
	memcpy(&value.ret, buffer + iSize, sizeof(value.ret));
	iSize += sizeof(value.ret);
	memcpy(&value.zone, buffer + iSize, sizeof(value.zone));
	iSize += sizeof(value.zone);
	return iSize;
}

int UnSerialization (char* buffer, st_STC_ConnectInfo& value)
{
	int iSize = 0;
	iSize += UnSerialization(buffer + iSize, value.info);
	return iSize;
}

int UnSerialization (char* buffer, st_STC_CreateChar& value)
{
	int iSize = 0;
	memcpy(&value.ID, buffer + iSize, sizeof(value.ID));
	iSize += sizeof(value.ID);
	iSize += UnSerialization(buffer + iSize, value.pos);
	return iSize;
}

int UnSerialization (char* buffer, st_STC_EnterZone& value)
{
	int iSize = 0;
	memcpy(&value.ret, buffer + iSize, sizeof(value.ret));
	iSize += sizeof(value.ret);
	memcpy(&value.Loop1, buffer + iSize, sizeof(value.Loop1));
	iSize += sizeof(value.Loop1);
	for(int i = 0; i < value.Loop1; ++i)
	{
		iSize += UnSerialization(buffer + iSize, value.info[i]);
	}
	return iSize;
}

int UnSerialization (char* buffer, st_STC_LeaveZone& value)
{
	int iSize = 0;
	memcpy(&value.zone, buffer + iSize, sizeof(value.zone));
	iSize += sizeof(value.zone);
	memcpy(&value.ID, buffer + iSize, sizeof(value.ID));
	iSize += sizeof(value.ID);
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

int UnSerialization (char* buffer, st_Vector& value)
{
	int iSize = 0;
	memcpy(&value.X, buffer + iSize, sizeof(value.X));
	iSize += sizeof(value.X);
	memcpy(&value.Y, buffer + iSize, sizeof(value.Y));
	iSize += sizeof(value.Y);
	return iSize;
}

