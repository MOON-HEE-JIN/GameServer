#pragma once

#include "StructDef.h" 

int Serialization(char* buffer, st_CTS_ChangePid& _value);
int Serialization(char* buffer, st_CTS_LoopBack& _value);
int Serialization(char* buffer, st_Header& _value);
int Serialization(char* buffer, st_STC_ChangePid& _value);
int Serialization(char* buffer, st_STC_LoopBack& _value);



int UnSerialization(char* buffer, st_CTS_ChangePid& _value);
int UnSerialization(char* buffer, st_CTS_LoopBack& _value);
int UnSerialization(char* buffer, st_Header& _value);
int UnSerialization(char* buffer, st_STC_ChangePid& _value);
int UnSerialization(char* buffer, st_STC_LoopBack& _value);
