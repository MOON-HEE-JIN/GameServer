#pragma once

#include <string>
struct st_Header
{
	__int32		type;
	__int32		size;
};
struct st_CTS_ChangePid
{
	__int32		pid;
};
struct st_CTS_LoopBack
{
	__int32		zone;
	__int64		data;
};
struct st_STC_ChangePid
{
	__int32		ret;
};
struct st_STC_LoopBack
{
	__int32		ret;
	__int32		zone;
	__int64		data;
};
