#pragma once

#include <string>
#include "ProjectDefineStruct.h"

namespace Rpc::ChatGame
{

struct st_Message
{
	__int16		length;
	std::string		comment;
};
struct st_CTS_GROUP
{
	__int32		group_ID;
	st_Message		msg;
};
struct st_CTS_HearBeat
{
	__int64		time;
};
struct st_CTS_NORMAL
{
	st_Message		msg;
};
struct st_CTS_WHISPER
{
	__int32		whisper_ID;
	st_Message		msg;
};
struct st_STC_GROUP
{
	__int32		group_ID;
	st_Message		msg;
};
struct st_STC_HearBeat
{
	__int64		time;
};
struct st_STC_NORMAL
{
	st_Message		msg;
};
struct st_STC_WHISPER
{
	__int32		whisper_ID;
	st_Message		msg;
};

} // namespace Rpc::ChatGame
