#pragma once

#include "ChatGame.StructDef.g.h" 

namespace Rpc::ChatGame
{

int Serialization(char* buffer, st_CTS_GROUP& _value);
int Serialization(char* buffer, st_CTS_HearBeat& _value);
int Serialization(char* buffer, st_CTS_NORMAL& _value);
int Serialization(char* buffer, st_CTS_WHISPER& _value);
int Serialization(char* buffer, st_Message& _value);
int Serialization(char* buffer, st_STC_GROUP& _value);
int Serialization(char* buffer, st_STC_HearBeat& _value);
int Serialization(char* buffer, st_STC_NORMAL& _value);
int Serialization(char* buffer, st_STC_WHISPER& _value);



int UnSerialization(char* buffer, st_CTS_GROUP& _value);
int UnSerialization(char* buffer, st_CTS_HearBeat& _value);
int UnSerialization(char* buffer, st_CTS_NORMAL& _value);
int UnSerialization(char* buffer, st_CTS_WHISPER& _value);
int UnSerialization(char* buffer, st_Message& _value);
int UnSerialization(char* buffer, st_STC_GROUP& _value);
int UnSerialization(char* buffer, st_STC_HearBeat& _value);
int UnSerialization(char* buffer, st_STC_NORMAL& _value);
int UnSerialization(char* buffer, st_STC_WHISPER& _value);

} // namespace Rpc::ChatGame
