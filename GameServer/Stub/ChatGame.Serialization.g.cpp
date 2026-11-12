#include "ChatGame.Serialization.g.h" 
#include "ChatGame.PacketEnumDef.g.h"
#include <memory.h>

namespace Rpc::ChatGame
{

int Serialization (char* buffer, st_CTS_GROUP& value)
{
	int hSize = 0;
	st_Header header;
	int iSize = sizeof(st_Header);
	memcpy(buffer + iSize, &value.group_ID, sizeof(value.group_ID));
	iSize += sizeof(value.group_ID);
	iSize += Serialization(buffer + iSize, value.msg);

	header.type = CHAT::GROUP;
	header.size = iSize - sizeof(st_Header);
	memcpy(buffer, &header, sizeof(st_Header));
	return iSize;
}

int Serialization (char* buffer, st_CTS_HearBeat& value)
{
	int hSize = 0;
	st_Header header;
	int iSize = sizeof(st_Header);
	memcpy(buffer + iSize, &value.time, sizeof(value.time));
	iSize += sizeof(value.time);

	header.type = CHAT::HEARTBEAT;
	header.size = iSize - sizeof(st_Header);
	memcpy(buffer, &header, sizeof(st_Header));
	return iSize;
}

int Serialization (char* buffer, st_CTS_NORMAL& value)
{
	int hSize = 0;
	st_Header header;
	int iSize = sizeof(st_Header);
	iSize += Serialization(buffer + iSize, value.msg);

	header.type = CHAT::NORMAL;
	header.size = iSize - sizeof(st_Header);
	memcpy(buffer, &header, sizeof(st_Header));
	return iSize;
}

int Serialization (char* buffer, st_CTS_WHISPER& value)
{
	int hSize = 0;
	st_Header header;
	int iSize = sizeof(st_Header);
	memcpy(buffer + iSize, &value.whisper_ID, sizeof(value.whisper_ID));
	iSize += sizeof(value.whisper_ID);
	iSize += Serialization(buffer + iSize, value.msg);

	header.type = CHAT::WHISPER;
	header.size = iSize - sizeof(st_Header);
	memcpy(buffer, &header, sizeof(st_Header));
	return iSize;
}

int Serialization (char* buffer, st_Message& value)
{
	int iSize = 0;
	memcpy(buffer + iSize, &value.length, sizeof(value.length));
	iSize += sizeof(value.length);
	memcpy(buffer + iSize, value.comment.c_str() , value.comment.length());
	iSize += value.comment.length();
	return iSize;
}

int Serialization (char* buffer, st_STC_GROUP& value)
{
	int hSize = 0;
	st_Header header;
	int iSize = sizeof(st_Header);
	memcpy(buffer + iSize, &value.group_ID, sizeof(value.group_ID));
	iSize += sizeof(value.group_ID);
	iSize += Serialization(buffer + iSize, value.msg);

	header.type = CHAT::GROUP;
	header.size = iSize - sizeof(st_Header);
	memcpy(buffer, &header, sizeof(st_Header));
	return iSize;
}

int Serialization (char* buffer, st_STC_HearBeat& value)
{
	int hSize = 0;
	st_Header header;
	int iSize = sizeof(st_Header);
	memcpy(buffer + iSize, &value.time, sizeof(value.time));
	iSize += sizeof(value.time);

	header.type = CHAT::HEARTBEAT;
	header.size = iSize - sizeof(st_Header);
	memcpy(buffer, &header, sizeof(st_Header));
	return iSize;
}

int Serialization (char* buffer, st_STC_NORMAL& value)
{
	int hSize = 0;
	st_Header header;
	int iSize = sizeof(st_Header);
	iSize += Serialization(buffer + iSize, value.msg);

	header.type = CHAT::NORMAL;
	header.size = iSize - sizeof(st_Header);
	memcpy(buffer, &header, sizeof(st_Header));
	return iSize;
}

int Serialization (char* buffer, st_STC_WHISPER& value)
{
	int hSize = 0;
	st_Header header;
	int iSize = sizeof(st_Header);
	memcpy(buffer + iSize, &value.whisper_ID, sizeof(value.whisper_ID));
	iSize += sizeof(value.whisper_ID);
	iSize += Serialization(buffer + iSize, value.msg);

	header.type = CHAT::WHISPER;
	header.size = iSize - sizeof(st_Header);
	memcpy(buffer, &header, sizeof(st_Header));
	return iSize;
}

int UnSerialization (char* buffer, st_CTS_GROUP& value)
{
	int iSize = 0;
	memcpy(&value.group_ID, buffer + iSize, sizeof(value.group_ID));
	iSize += sizeof(value.group_ID);
	iSize += UnSerialization(buffer + iSize, value.msg);
	return iSize;
}

int UnSerialization (char* buffer, st_CTS_HearBeat& value)
{
	int iSize = 0;
	memcpy(&value.time, buffer + iSize, sizeof(value.time));
	iSize += sizeof(value.time);
	return iSize;
}

int UnSerialization (char* buffer, st_CTS_NORMAL& value)
{
	int iSize = 0;
	iSize += UnSerialization(buffer + iSize, value.msg);
	return iSize;
}

int UnSerialization (char* buffer, st_CTS_WHISPER& value)
{
	int iSize = 0;
	memcpy(&value.whisper_ID, buffer + iSize, sizeof(value.whisper_ID));
	iSize += sizeof(value.whisper_ID);
	iSize += UnSerialization(buffer + iSize, value.msg);
	return iSize;
}

int UnSerialization (char* buffer, st_Message& value)
{
	int iSize = 0;
	memcpy(&value.length, buffer + iSize, sizeof(value.length));
	iSize += sizeof(value.length);
	value.comment.assign(buffer + iSize, value.length);
	iSize += value.length;
	return iSize;
}

int UnSerialization (char* buffer, st_STC_GROUP& value)
{
	int iSize = 0;
	memcpy(&value.group_ID, buffer + iSize, sizeof(value.group_ID));
	iSize += sizeof(value.group_ID);
	iSize += UnSerialization(buffer + iSize, value.msg);
	return iSize;
}

int UnSerialization (char* buffer, st_STC_HearBeat& value)
{
	int iSize = 0;
	memcpy(&value.time, buffer + iSize, sizeof(value.time));
	iSize += sizeof(value.time);
	return iSize;
}

int UnSerialization (char* buffer, st_STC_NORMAL& value)
{
	int iSize = 0;
	iSize += UnSerialization(buffer + iSize, value.msg);
	return iSize;
}

int UnSerialization (char* buffer, st_STC_WHISPER& value)
{
	int iSize = 0;
	memcpy(&value.whisper_ID, buffer + iSize, sizeof(value.whisper_ID));
	iSize += sizeof(value.whisper_ID);
	iSize += UnSerialization(buffer + iSize, value.msg);
	return iSize;
}


} // namespace Rpc::ChatGame
