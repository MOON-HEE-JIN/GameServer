#pragma once

#include "StructDef.h" 

int Serialization(char* buffer, st_CTS_ChangeZone& _value);
int Serialization(char* buffer, st_CTS_EnterZone& _value);
int Serialization(char* buffer, st_CTS_LoopBack& _value);
int Serialization(char* buffer, st_CTS_MoveStart& _value);
int Serialization(char* buffer, st_CTS_MoveStop& _value);
int Serialization(char* buffer, st_CTS_ObserverConnect& _value);
int Serialization(char* buffer, st_CTS_Teleport& _value);
int Serialization(char* buffer, st_ConnectInfo& _value);
int Serialization(char* buffer, st_EntityInfo& _value);
static int Serialization(char* buffer, st_Header& _value);
int Serialization(char* buffer, st_Msg& _value);
int Serialization(char* buffer, st_STC_ChangeZone& _value);
int Serialization(char* buffer, st_STC_ChangeingZone& _value);
int Serialization(char* buffer, st_STC_ConnectInfo& _value);
int Serialization(char* buffer, st_STC_CreateChar& _value);
int Serialization(char* buffer, st_STC_EnterZone& _value);
int Serialization(char* buffer, st_STC_LeaveZone& _value);
int Serialization(char* buffer, st_STC_LoopBack& _value);
int Serialization(char* buffer, st_STC_MoveStart& _value);
int Serialization(char* buffer, st_STC_MoveStop& _value);
int Serialization(char* buffer, st_STC_ObserverConnect& _value);
int Serialization(char* buffer, st_STC_Teleport& _value);
int Serialization(char* buffer, st_String& _value);
static int Serialization(char* buffer, st_Vector3F& _value);
static int Serialization(char* buffer, st_Vector3L& _value);



int UnSerialization(char* buffer, st_CTS_ChangeZone& _value);
int UnSerialization(char* buffer, st_CTS_EnterZone& _value);
int UnSerialization(char* buffer, st_CTS_LoopBack& _value);
int UnSerialization(char* buffer, st_CTS_MoveStart& _value);
int UnSerialization(char* buffer, st_CTS_MoveStop& _value);
int UnSerialization(char* buffer, st_CTS_ObserverConnect& _value);
int UnSerialization(char* buffer, st_CTS_Teleport& _value);
int UnSerialization(char* buffer, st_ConnectInfo& _value);
int UnSerialization(char* buffer, st_EntityInfo& _value);
int UnSerialization(char* buffer, st_Header& _value);
int UnSerialization(char* buffer, st_Msg& _value);
int UnSerialization(char* buffer, st_STC_ChangeZone& _value);
int UnSerialization(char* buffer, st_STC_ChangeingZone& _value);
int UnSerialization(char* buffer, st_STC_ConnectInfo& _value);
int UnSerialization(char* buffer, st_STC_CreateChar& _value);
int UnSerialization(char* buffer, st_STC_EnterZone& _value);
int UnSerialization(char* buffer, st_STC_LeaveZone& _value);
int UnSerialization(char* buffer, st_STC_LoopBack& _value);
int UnSerialization(char* buffer, st_STC_MoveStart& _value);
int UnSerialization(char* buffer, st_STC_MoveStop& _value);
int UnSerialization(char* buffer, st_STC_ObserverConnect& _value);
int UnSerialization(char* buffer, st_STC_Teleport& _value);
int UnSerialization(char* buffer, st_String& _value);
int UnSerialization(char* buffer, st_Vector3F& _value);
int UnSerialization(char* buffer, st_Vector3L& _value);
