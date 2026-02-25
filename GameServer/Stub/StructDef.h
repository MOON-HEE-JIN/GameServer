#pragma once

#include <string>
struct st_Header
{
	__int32		type;
	__int32		size;
};
struct st_Vector
{
	float		X;
	float		Y;
};
struct st_EntityInfo
{
	__int32		type;
	__int32		ID;
	st_Vector		pos;
};
struct st_ConnectInfo
{
	__int32		ID;
};
struct st_CTS_ChangeZone
{
	__int32		zone;
};
struct st_CTS_EnterZone
{
	__int32		zone;
};
struct st_CTS_LoopBack
{
	__int32		zone;
	__int64		data;
};
struct st_STC_ChangeZone
{
	__int32		ret;
	__int32		zone;
};
struct st_STC_ConnectInfo
{
	st_ConnectInfo		info;
};
struct st_STC_CreateChar
{
	__int32		ID;
	st_Vector		pos;
};
struct st_STC_EnterZone
{
	__int32		ret;
	__int32		Loop1;
	st_EntityInfo		info[50];
};
struct st_STC_LeaveZone
{
	__int32		zone;
	__int32		ID;
};
struct st_STC_LoopBack
{
	__int32		ret;
	__int32		zone;
	__int64		data;
};
