#pragma once

#include <string>
#include "ProjectDefineStruct.h"
struct st_EntityInfo
{
	__int32		type;
	__int32		ID;
	st_Vector3F		pos;
};
struct st_ConnectInfo
{
	__int32		ID;
};
struct st_String
{
	__int16		length;
	std::string		comment;
};
struct st_Msg
{
	__int32		type;
	st_String		Message;
};
struct st_CTS_ChangeZone
{
	__int32		channel;
	__int32		zone;
};
struct st_CTS_EnterZone
{
	__int32		channel;
	__int32		zone;
};
struct st_CTS_LoopBack
{
	__int64		data;
};
struct st_CTS_MoveStart
{
	st_Vector3F		dir;
	st_Vector3F		goal;
	st_Vector3F		pos;
};
struct st_CTS_MoveStop
{
	st_Vector3F		pos;
};
struct st_CTS_ObserverConnect
{
	__int32		ID;
	__int32		zone;
};
struct st_CTS_Teleport
{
	st_Vector3F		pos;
};
struct st_STC_ChangeZone
{
	__int32		ret;
	__int32		channel;
	__int32		zone;
};
struct st_STC_ChangeingZone
{
	__int32		ret;
	__int32		type;
};
struct st_STC_ConnectInfo
{
	st_ConnectInfo		info;
};
struct st_STC_CreateChar
{
	__int32		ID;
	st_Vector3F		pos;
	float		speed;
};
struct st_STC_EnterZone
{
	__int32		ret;
	__int32		Loop1;
	st_EntityInfo		info[50];
};
struct st_STC_LeaveZone
{
	__int32		channel;
	__int32		zone;
	__int32		ID;
};
struct st_STC_LoopBack
{
	__int32		ret;
	__int64		data;
};
struct st_STC_MoveStart
{
	__int32		ret;
	st_Vector3F		pos;
};
struct st_STC_MoveStop
{
	__int32		ret;
	__int32		type;
	__int32		ID;
	st_Vector3F		pos;
};
struct st_STC_ObserverConnect
{
	__int32		ret;
};
struct st_STC_Teleport
{
	__int32		ret;
};
