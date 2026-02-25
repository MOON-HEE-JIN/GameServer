#pragma once

#include "StructDef.h" 

int Serialization(char* buffer, st_CTS_ChangeZone& _value);
int Serialization(char* buffer, st_CTS_EnterZone& _value);
int Serialization(char* buffer, st_CTS_LoopBack& _value);
int Serialization(char* buffer, st_ConnectInfo& _value);
int Serialization(char* buffer, st_EntityInfo& _value);
int Serialization(char* buffer, st_Header& _value);
int Serialization(char* buffer, st_STC_ChangeZone& _value);
int Serialization(char* buffer, st_STC_ConnectInfo& _value);
int Serialization(char* buffer, st_STC_CreateChar& _value);
int Serialization(char* buffer, st_STC_EnterZone& _value);
int Serialization(char* buffer, st_STC_LeaveZone& _value);
int Serialization(char* buffer, st_STC_LoopBack& _value);
int Serialization(char* buffer, st_Vector& _value);



int UnSerialization(char* buffer, st_CTS_ChangeZone& _value);
int UnSerialization(char* buffer, st_CTS_EnterZone& _value);
int UnSerialization(char* buffer, st_CTS_LoopBack& _value);
int UnSerialization(char* buffer, st_ConnectInfo& _value);
int UnSerialization(char* buffer, st_EntityInfo& _value);
int UnSerialization(char* buffer, st_Header& _value);
int UnSerialization(char* buffer, st_STC_ChangeZone& _value);
int UnSerialization(char* buffer, st_STC_ConnectInfo& _value);
int UnSerialization(char* buffer, st_STC_CreateChar& _value);
int UnSerialization(char* buffer, st_STC_EnterZone& _value);
int UnSerialization(char* buffer, st_STC_LeaveZone& _value);
int UnSerialization(char* buffer, st_STC_LoopBack& _value);
int UnSerialization(char* buffer, st_Vector& _value);
