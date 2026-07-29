#include "Serialization.h" 
#include "PacketEnumDef.h"
#include <memory.h>

int Serialization (char* buffer, st_CTS_ChangeZone& value)
{
	int hSize = 0;
	st_Header header;
	int iSize = sizeof(st_Header);
	memcpy(buffer + iSize, &value.channel, sizeof(value.channel));
	iSize += sizeof(value.channel);
	memcpy(buffer + iSize, &value.zone, sizeof(value.zone));
	iSize += sizeof(value.zone);

	header.type = GAME::CHANGEZONE;
	header.size = iSize - sizeof(st_Header);
	Serialization(buffer, header);
	return iSize;
}

int Serialization (char* buffer, st_CTS_LoopBack& value)
{
	int hSize = 0;
	st_Header header;
	int iSize = sizeof(st_Header);
	memcpy(buffer + iSize, &value.data, sizeof(value.data));
	iSize += sizeof(value.data);

	header.type = GAME::LOOPBACK;
	header.size = iSize - sizeof(st_Header);
	Serialization(buffer, header);
	return iSize;
}

int Serialization (char* buffer, st_CTS_MoveStart& value)
{
	int hSize = 0;
	st_Header header;
	int iSize = sizeof(st_Header);
	iSize += Serialization(buffer + iSize, value.dir);
	iSize += Serialization(buffer + iSize, value.goal);
	iSize += Serialization(buffer + iSize, value.pos);

	header.type = GAME::MOVESTART;
	header.size = iSize - sizeof(st_Header);
	Serialization(buffer, header);
	return iSize;
}

int Serialization (char* buffer, st_CTS_MoveStop& value)
{
	int hSize = 0;
	st_Header header;
	int iSize = sizeof(st_Header);
	iSize += Serialization(buffer + iSize, value.pos);

	header.type = GAME::MOVESTOP;
	header.size = iSize - sizeof(st_Header);
	Serialization(buffer, header);
	return iSize;
}

int Serialization (char* buffer, st_CTS_ObserverConnect& value)
{
	int hSize = 0;
	st_Header header;
	int iSize = sizeof(st_Header);
	memcpy(buffer + iSize, &value.ID, sizeof(value.ID));
	iSize += sizeof(value.ID);
	memcpy(buffer + iSize, &value.zone, sizeof(value.zone));
	iSize += sizeof(value.zone);

	header.type = OBSERVER::CONNET_OBSERVER;
	header.size = iSize - sizeof(st_Header);
	Serialization(buffer, header);
	return iSize;
}

int Serialization (char* buffer, st_CTS_Teleport& value)
{
	int hSize = 0;
	st_Header header;
	int iSize = sizeof(st_Header);
	iSize += Serialization(buffer + iSize, value.pos);

	header.type = GAME::TELEPORT;
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

int Serialization (char* buffer, st_Msg& value)
{
	int iSize = 0;
	memcpy(buffer + iSize, &value.type, sizeof(value.type));
	iSize += sizeof(value.type);
	iSize += Serialization(buffer + iSize, value.Message);
	return iSize;
}

int Serialization (char* buffer, st_PlayerInfo& value)
{
	int iSize = 0;
	memcpy(buffer + iSize, &value.ID, sizeof(value.ID));
	iSize += sizeof(value.ID);
	iSize += Serialization(buffer + iSize, value.pos);
	memcpy(buffer + iSize, &value.speed, sizeof(value.speed));
	iSize += sizeof(value.speed);
	return iSize;
}

int Serialization (char* buffer, st_STC_AoiInPlayer& value)
{
	int hSize = 0;
	st_Header header;
	int iSize = sizeof(st_Header);
	iSize += Serialization(buffer + iSize, value.info);

	header.type = GAME::AOI_IN_PLAYER;
	header.size = iSize - sizeof(st_Header);
	Serialization(buffer, header);
	return iSize;
}

int Serialization (char* buffer, st_STC_AoiInPlayers& value)
{
	int hSize = 0;
	st_Header header;
	int iSize = sizeof(st_Header);
	memcpy(buffer + iSize, &value.Loop1, sizeof(value.Loop1));
	iSize += sizeof(value.Loop1);
	for(int i = 0; i < value.Loop1; ++i)
	{
		iSize += Serialization(buffer + iSize, value.info[i]);
	}

	header.type = GAME::AOI_IN_PLAYERS;
	header.size = iSize - sizeof(st_Header);
	Serialization(buffer, header);
	return iSize;
}

int Serialization (char* buffer, st_STC_AoiOutPlayer& value)
{
	int hSize = 0;
	st_Header header;
	int iSize = sizeof(st_Header);
	memcpy(buffer + iSize, &value.ID, sizeof(value.ID));
	iSize += sizeof(value.ID);

	header.type = GAME::AOI_OUT_PLAYER;
	header.size = iSize - sizeof(st_Header);
	Serialization(buffer, header);
	return iSize;
}

int Serialization (char* buffer, st_STC_AoiOutPlayers& value)
{
	int hSize = 0;
	st_Header header;
	int iSize = sizeof(st_Header);
	memcpy(buffer + iSize, &value.Loop1, sizeof(value.Loop1));
	iSize += sizeof(value.Loop1);
	for(int i = 0; i < value.Loop1; ++i)
	{
		iSize += Serialization(buffer + iSize, value.info[i]);
	}

	header.type = GAME::AOI_OUT_PLAYERS;
	header.size = iSize - sizeof(st_Header);
	Serialization(buffer, header);
	return iSize;
}

int Serialization (char* buffer, st_STC_ChangeZone& value)
{
	int hSize = 0;
	st_Header header;
	int iSize = sizeof(st_Header);
	memcpy(buffer + iSize, &value.ret, sizeof(value.ret));
	iSize += sizeof(value.ret);
	memcpy(buffer + iSize, &value.channel, sizeof(value.channel));
	iSize += sizeof(value.channel);
	memcpy(buffer + iSize, &value.zone, sizeof(value.zone));
	iSize += sizeof(value.zone);

	header.type = GAME::CHANGEZONE;
	header.size = iSize - sizeof(st_Header);
	Serialization(buffer, header);
	return iSize;
}

int Serialization (char* buffer, st_STC_ChangeingZone& value)
{
	int hSize = 0;
	st_Header header;
	int iSize = sizeof(st_Header);
	memcpy(buffer + iSize, &value.ret, sizeof(value.ret));
	iSize += sizeof(value.ret);
	memcpy(buffer + iSize, &value.type, sizeof(value.type));
	iSize += sizeof(value.type);

	header.type = GAME::CHANGEINGZONE;
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

int Serialization (char* buffer, st_STC_LoopBack& value)
{
	int hSize = 0;
	st_Header header;
	int iSize = sizeof(st_Header);
	memcpy(buffer + iSize, &value.ret, sizeof(value.ret));
	iSize += sizeof(value.ret);
	memcpy(buffer + iSize, &value.data, sizeof(value.data));
	iSize += sizeof(value.data);

	header.type = GAME::LOOPBACK;
	header.size = iSize - sizeof(st_Header);
	Serialization(buffer, header);
	return iSize;
}

int Serialization (char* buffer, st_STC_MoveStart& value)
{
	int hSize = 0;
	st_Header header;
	int iSize = sizeof(st_Header);
	memcpy(buffer + iSize, &value.ret, sizeof(value.ret));
	iSize += sizeof(value.ret);
	iSize += Serialization(buffer + iSize, value.pos);

	header.type = GAME::MOVESTART;
	header.size = iSize - sizeof(st_Header);
	Serialization(buffer, header);
	return iSize;
}

int Serialization (char* buffer, st_STC_MoveStop& value)
{
	int hSize = 0;
	st_Header header;
	int iSize = sizeof(st_Header);
	memcpy(buffer + iSize, &value.ret, sizeof(value.ret));
	iSize += sizeof(value.ret);
	memcpy(buffer + iSize, &value.type, sizeof(value.type));
	iSize += sizeof(value.type);
	memcpy(buffer + iSize, &value.ID, sizeof(value.ID));
	iSize += sizeof(value.ID);
	iSize += Serialization(buffer + iSize, value.pos);

	header.type = GAME::MOVESTOP;
	header.size = iSize - sizeof(st_Header);
	Serialization(buffer, header);
	return iSize;
}

int Serialization (char* buffer, st_STC_ObserverConnect& value)
{
	int hSize = 0;
	st_Header header;
	int iSize = sizeof(st_Header);
	memcpy(buffer + iSize, &value.ret, sizeof(value.ret));
	iSize += sizeof(value.ret);

	header.type = OBSERVER::CONNET_OBSERVER;
	header.size = iSize - sizeof(st_Header);
	Serialization(buffer, header);
	return iSize;
}

int Serialization (char* buffer, st_STC_OtherMoveStart& value)
{
	int hSize = 0;
	st_Header header;
	int iSize = sizeof(st_Header);
	memcpy(buffer + iSize, &value.type, sizeof(value.type));
	iSize += sizeof(value.type);
	memcpy(buffer + iSize, &value.ID, sizeof(value.ID));
	iSize += sizeof(value.ID);
	iSize += Serialization(buffer + iSize, value.pos);
	iSize += Serialization(buffer + iSize, value.dir);

	header.type = GAME::OTHERMOVESTART;
	header.size = iSize - sizeof(st_Header);
	Serialization(buffer, header);
	return iSize;
}

int Serialization (char* buffer, st_STC_Teleport& value)
{
	int hSize = 0;
	st_Header header;
	int iSize = sizeof(st_Header);
	memcpy(buffer + iSize, &value.ret, sizeof(value.ret));
	iSize += sizeof(value.ret);
	iSize += Serialization(buffer + iSize, value.pos);

	header.type = GAME::TELEPORT;
	header.size = iSize - sizeof(st_Header);
	Serialization(buffer, header);
	return iSize;
}

int Serialization (char* buffer, st_String& value)
{
	int iSize = 0;
	memcpy(buffer + iSize, &value.length, sizeof(value.length));
	iSize += sizeof(value.length);
	memcpy(buffer + iSize, value.comment.c_str() , value.comment.length());
	iSize += value.comment.length();
	return iSize;
}

int Serialization (char* buffer, st_Vector3F& value)
{
	int iSize = 0;
	memcpy(buffer + iSize, &value.X, sizeof(value.X));
	iSize += sizeof(value.X);
	memcpy(buffer + iSize, &value.Y, sizeof(value.Y));
	iSize += sizeof(value.Y);
	memcpy(buffer + iSize, &value.Z, sizeof(value.Z));
	iSize += sizeof(value.Z);
	return iSize;
}

int UnSerialization (char* buffer, st_CTS_ChangeZone& value)
{
	int iSize = 0;
	memcpy(&value.channel, buffer + iSize, sizeof(value.channel));
	iSize += sizeof(value.channel);
	memcpy(&value.zone, buffer + iSize, sizeof(value.zone));
	iSize += sizeof(value.zone);
	return iSize;
}

int UnSerialization (char* buffer, st_CTS_LoopBack& value)
{
	int iSize = 0;
	memcpy(&value.data, buffer + iSize, sizeof(value.data));
	iSize += sizeof(value.data);
	return iSize;
}

int UnSerialization (char* buffer, st_CTS_MoveStart& value)
{
	int iSize = 0;
	iSize += UnSerialization(buffer + iSize, value.dir);
	iSize += UnSerialization(buffer + iSize, value.goal);
	iSize += UnSerialization(buffer + iSize, value.pos);
	return iSize;
}

int UnSerialization (char* buffer, st_CTS_MoveStop& value)
{
	int iSize = 0;
	iSize += UnSerialization(buffer + iSize, value.pos);
	return iSize;
}

int UnSerialization (char* buffer, st_CTS_ObserverConnect& value)
{
	int iSize = 0;
	memcpy(&value.ID, buffer + iSize, sizeof(value.ID));
	iSize += sizeof(value.ID);
	memcpy(&value.zone, buffer + iSize, sizeof(value.zone));
	iSize += sizeof(value.zone);
	return iSize;
}

int UnSerialization (char* buffer, st_CTS_Teleport& value)
{
	int iSize = 0;
	iSize += UnSerialization(buffer + iSize, value.pos);
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

int UnSerialization (char* buffer, st_Msg& value)
{
	int iSize = 0;
	memcpy(&value.type, buffer + iSize, sizeof(value.type));
	iSize += sizeof(value.type);
	iSize += UnSerialization(buffer + iSize, value.Message);
	return iSize;
}

int UnSerialization (char* buffer, st_PlayerInfo& value)
{
	int iSize = 0;
	memcpy(&value.ID, buffer + iSize, sizeof(value.ID));
	iSize += sizeof(value.ID);
	iSize += UnSerialization(buffer + iSize, value.pos);
	memcpy(&value.speed, buffer + iSize, sizeof(value.speed));
	iSize += sizeof(value.speed);
	return iSize;
}

int UnSerialization (char* buffer, st_STC_AoiInPlayer& value)
{
	int iSize = 0;
	iSize += UnSerialization(buffer + iSize, value.info);
	return iSize;
}

int UnSerialization (char* buffer, st_STC_AoiInPlayers& value)
{
	int iSize = 0;
	memcpy(&value.Loop1, buffer + iSize, sizeof(value.Loop1));
	iSize += sizeof(value.Loop1);
	for(int i = 0; i < value.Loop1; ++i)
	{
		iSize += UnSerialization(buffer + iSize, value.info[i]);
	}
	return iSize;
}

int UnSerialization (char* buffer, st_STC_AoiOutPlayer& value)
{
	int iSize = 0;
	memcpy(&value.ID, buffer + iSize, sizeof(value.ID));
	iSize += sizeof(value.ID);
	return iSize;
}

int UnSerialization (char* buffer, st_STC_AoiOutPlayers& value)
{
	int iSize = 0;
	memcpy(&value.Loop1, buffer + iSize, sizeof(value.Loop1));
	iSize += sizeof(value.Loop1);
	for(int i = 0; i < value.Loop1; ++i)
	{
		iSize += UnSerialization(buffer + iSize, value.info[i]);
	}
	return iSize;
}

int UnSerialization (char* buffer, st_STC_ChangeZone& value)
{
	int iSize = 0;
	memcpy(&value.ret, buffer + iSize, sizeof(value.ret));
	iSize += sizeof(value.ret);
	memcpy(&value.channel, buffer + iSize, sizeof(value.channel));
	iSize += sizeof(value.channel);
	memcpy(&value.zone, buffer + iSize, sizeof(value.zone));
	iSize += sizeof(value.zone);
	return iSize;
}

int UnSerialization (char* buffer, st_STC_ChangeingZone& value)
{
	int iSize = 0;
	memcpy(&value.ret, buffer + iSize, sizeof(value.ret));
	iSize += sizeof(value.ret);
	memcpy(&value.type, buffer + iSize, sizeof(value.type));
	iSize += sizeof(value.type);
	return iSize;
}

int UnSerialization (char* buffer, st_STC_ConnectInfo& value)
{
	int iSize = 0;
	iSize += UnSerialization(buffer + iSize, value.info);
	return iSize;
}

int UnSerialization (char* buffer, st_STC_LoopBack& value)
{
	int iSize = 0;
	memcpy(&value.ret, buffer + iSize, sizeof(value.ret));
	iSize += sizeof(value.ret);
	memcpy(&value.data, buffer + iSize, sizeof(value.data));
	iSize += sizeof(value.data);
	return iSize;
}

int UnSerialization (char* buffer, st_STC_MoveStart& value)
{
	int iSize = 0;
	memcpy(&value.ret, buffer + iSize, sizeof(value.ret));
	iSize += sizeof(value.ret);
	iSize += UnSerialization(buffer + iSize, value.pos);
	return iSize;
}

int UnSerialization (char* buffer, st_STC_MoveStop& value)
{
	int iSize = 0;
	memcpy(&value.ret, buffer + iSize, sizeof(value.ret));
	iSize += sizeof(value.ret);
	memcpy(&value.type, buffer + iSize, sizeof(value.type));
	iSize += sizeof(value.type);
	memcpy(&value.ID, buffer + iSize, sizeof(value.ID));
	iSize += sizeof(value.ID);
	iSize += UnSerialization(buffer + iSize, value.pos);
	return iSize;
}

int UnSerialization (char* buffer, st_STC_ObserverConnect& value)
{
	int iSize = 0;
	memcpy(&value.ret, buffer + iSize, sizeof(value.ret));
	iSize += sizeof(value.ret);
	return iSize;
}

int UnSerialization (char* buffer, st_STC_OtherMoveStart& value)
{
	int iSize = 0;
	memcpy(&value.type, buffer + iSize, sizeof(value.type));
	iSize += sizeof(value.type);
	memcpy(&value.ID, buffer + iSize, sizeof(value.ID));
	iSize += sizeof(value.ID);
	iSize += UnSerialization(buffer + iSize, value.pos);
	iSize += UnSerialization(buffer + iSize, value.dir);
	return iSize;
}

int UnSerialization (char* buffer, st_STC_Teleport& value)
{
	int iSize = 0;
	memcpy(&value.ret, buffer + iSize, sizeof(value.ret));
	iSize += sizeof(value.ret);
	iSize += UnSerialization(buffer + iSize, value.pos);
	return iSize;
}

int UnSerialization (char* buffer, st_String& value)
{
	int iSize = 0;
	memcpy(&value.length, buffer + iSize, sizeof(value.length));
	iSize += sizeof(value.length);
	value.comment.assign(buffer + iSize, value.length);
	iSize += value.length;
	return iSize;
}

int UnSerialization (char* buffer, st_Vector3F& value)
{
	int iSize = 0;
	memcpy(&value.X, buffer + iSize, sizeof(value.X));
	iSize += sizeof(value.X);
	memcpy(&value.Y, buffer + iSize, sizeof(value.Y));
	iSize += sizeof(value.Y);
	memcpy(&value.Z, buffer + iSize, sizeof(value.Z));
	iSize += sizeof(value.Z);
	return iSize;
}

