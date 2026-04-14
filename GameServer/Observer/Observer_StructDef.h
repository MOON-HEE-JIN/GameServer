#pragma once

#include <string>
#include "ProjectDefineStruct.h"
struct st_String
{
	__int16		length;
	std::string		msg;
};
struct st_Msg
{
	__int32		MsgID;
	st_String		Message;
};
struct st_CTS_ENTER_ZONE
{
	__int32		ID;
};
struct st_CTS_EXIT_ZONE
{
	__int32		ID;
};
struct st_CTS_HEARTBEAT
{
	__int32		Number;
};
struct st_CTS_MESSAGE
{
	__int32		ID;
	st_Msg		msg;
};
struct st_STC_ENTER_ZONE
{
	__int32		ret;
	__int32		ID;
};
struct st_STC_EXIT_ZONE
{
	__int32		ret;
	__int32		ID;
};
struct st_STC_HEARTBEAT
{
	__int32		Number;
};
struct st_STC_MESSAGE
{
	__int32		ret;
	st_Msg		msg;
};
