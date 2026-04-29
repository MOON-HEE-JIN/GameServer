#include "Observer_Serialization.h" 
#include "Observer_PacketEnumDef.h"
#include <memory.h>

int Serialization (char* buffer, st_CTS_ENTER_ZONE& value)
{
	int hSize = 0;
	st_Header header;
	int iSize = sizeof(st_Header);
	memcpy(buffer + iSize, &value.ID, sizeof(value.ID));
	iSize += sizeof(value.ID);

	header.type = OBSERVER::ENTER_ZONE;
	header.size = iSize - sizeof(st_Header);
	Serialization(buffer, header);
	return iSize;
}

int Serialization (char* buffer, st_CTS_EXIT_ZONE& value)
{
	int hSize = 0;
	st_Header header;
	int iSize = sizeof(st_Header);
	memcpy(buffer + iSize, &value.ID, sizeof(value.ID));
	iSize += sizeof(value.ID);

	header.type = OBSERVER::EXIT_ZONE;
	header.size = iSize - sizeof(st_Header);
	Serialization(buffer, header);
	return iSize;
}

int Serialization (char* buffer, st_CTS_HEARTBEAT& value)
{
	int hSize = 0;
	st_Header header;
	int iSize = sizeof(st_Header);
	memcpy(buffer + iSize, &value.Number, sizeof(value.Number));
	iSize += sizeof(value.Number);

	header.type = OBSERVER::HEARTBEAT;
	header.size = iSize - sizeof(st_Header);
	Serialization(buffer, header);
	return iSize;
}

int Serialization (char* buffer, st_CTS_MESSAGE& value)
{
	int hSize = 0;
	st_Header header;
	int iSize = sizeof(st_Header);
	memcpy(buffer + iSize, &value.ID, sizeof(value.ID));
	iSize += sizeof(value.ID);
	iSize += Serialization(buffer + iSize, value.msg);

	header.type = OBSERVER::MESSAGE;
	header.size = iSize - sizeof(st_Header);
	Serialization(buffer, header);
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

int Serialization (char* buffer, st_Msg& value)
{
	int iSize = 0;
	memcpy(buffer + iSize, &value.MsgID, sizeof(value.MsgID));
	iSize += sizeof(value.MsgID);
	iSize += Serialization(buffer + iSize, value.Message);
	return iSize;
}

int Serialization (char* buffer, st_STC_ENTER_ZONE& value)
{
	int hSize = 0;
	st_Header header;
	int iSize = sizeof(st_Header);
	memcpy(buffer + iSize, &value.ret, sizeof(value.ret));
	iSize += sizeof(value.ret);
	memcpy(buffer + iSize, &value.ID, sizeof(value.ID));
	iSize += sizeof(value.ID);

	header.type = OBSERVER::ENTER_ZONE;
	header.size = iSize - sizeof(st_Header);
	Serialization(buffer, header);
	return iSize;
}

int Serialization (char* buffer, st_STC_EXIT_ZONE& value)
{
	int hSize = 0;
	st_Header header;
	int iSize = sizeof(st_Header);
	memcpy(buffer + iSize, &value.ret, sizeof(value.ret));
	iSize += sizeof(value.ret);
	memcpy(buffer + iSize, &value.ID, sizeof(value.ID));
	iSize += sizeof(value.ID);

	header.type = OBSERVER::EXIT_ZONE;
	header.size = iSize - sizeof(st_Header);
	Serialization(buffer, header);
	return iSize;
}

int Serialization (char* buffer, st_STC_HEARTBEAT& value)
{
	int hSize = 0;
	st_Header header;
	int iSize = sizeof(st_Header);
	memcpy(buffer + iSize, &value.Number, sizeof(value.Number));
	iSize += sizeof(value.Number);

	header.type = OBSERVER::HEARTBEAT;
	header.size = iSize - sizeof(st_Header);
	Serialization(buffer, header);
	return iSize;
}

int Serialization (char* buffer, st_STC_MESSAGE& value)
{
	int hSize = 0;
	st_Header header;
	int iSize = sizeof(st_Header);
	memcpy(buffer + iSize, &value.ret, sizeof(value.ret));
	iSize += sizeof(value.ret);
	iSize += Serialization(buffer + iSize, value.msg);

	header.type = OBSERVER::MESSAGE;
	header.size = iSize - sizeof(st_Header);
	Serialization(buffer, header);
	return iSize;
}

int Serialization (char* buffer, st_String& value)
{
	int iSize = 0;
	memcpy(buffer + iSize, &value.length, sizeof(value.length));
	iSize += sizeof(value.length);
	memcpy(buffer + iSize, value.msg.c_str() , value.msg.length());
	iSize += value.msg.length();
	return iSize;
}

int UnSerialization (char* buffer, st_CTS_ENTER_ZONE& value)
{
	int iSize = 0;
	memcpy(&value.ID, buffer + iSize, sizeof(value.ID));
	iSize += sizeof(value.ID);
	return iSize;
}

int UnSerialization (char* buffer, st_CTS_EXIT_ZONE& value)
{
	int iSize = 0;
	memcpy(&value.ID, buffer + iSize, sizeof(value.ID));
	iSize += sizeof(value.ID);
	return iSize;
}

int UnSerialization (char* buffer, st_CTS_HEARTBEAT& value)
{
	int iSize = 0;
	memcpy(&value.Number, buffer + iSize, sizeof(value.Number));
	iSize += sizeof(value.Number);
	return iSize;
}

int UnSerialization (char* buffer, st_CTS_MESSAGE& value)
{
	int iSize = 0;
	memcpy(&value.ID, buffer + iSize, sizeof(value.ID));
	iSize += sizeof(value.ID);
	iSize += UnSerialization(buffer + iSize, value.msg);
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

int UnSerialization (char* buffer, st_Msg& value)
{
	int iSize = 0;
	memcpy(&value.MsgID, buffer + iSize, sizeof(value.MsgID));
	iSize += sizeof(value.MsgID);
	iSize += UnSerialization(buffer + iSize, value.Message);
	return iSize;
}

int UnSerialization (char* buffer, st_STC_ENTER_ZONE& value)
{
	int iSize = 0;
	memcpy(&value.ret, buffer + iSize, sizeof(value.ret));
	iSize += sizeof(value.ret);
	memcpy(&value.ID, buffer + iSize, sizeof(value.ID));
	iSize += sizeof(value.ID);
	return iSize;
}

int UnSerialization (char* buffer, st_STC_EXIT_ZONE& value)
{
	int iSize = 0;
	memcpy(&value.ret, buffer + iSize, sizeof(value.ret));
	iSize += sizeof(value.ret);
	memcpy(&value.ID, buffer + iSize, sizeof(value.ID));
	iSize += sizeof(value.ID);
	return iSize;
}

int UnSerialization (char* buffer, st_STC_HEARTBEAT& value)
{
	int iSize = 0;
	memcpy(&value.Number, buffer + iSize, sizeof(value.Number));
	iSize += sizeof(value.Number);
	return iSize;
}

int UnSerialization (char* buffer, st_STC_MESSAGE& value)
{
	int iSize = 0;
	memcpy(&value.ret, buffer + iSize, sizeof(value.ret));
	iSize += sizeof(value.ret);
	iSize += UnSerialization(buffer + iSize, value.msg);
	return iSize;
}

int UnSerialization (char* buffer, st_String& value)
{
	int iSize = 0;
	memcpy(&value.length, buffer + iSize, sizeof(value.length));
	iSize += sizeof(value.length);
	value.msg.assign(buffer + iSize, value.length);
	iSize += value.length;
	return iSize;
}

