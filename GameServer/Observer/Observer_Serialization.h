#pragma once

#include "Observer_StructDef.h" 

int Serialization(char* buffer, st_CTS_ENTER_ZONE& _value);
int Serialization(char* buffer, st_CTS_EXIT_ZONE& _value);
int Serialization(char* buffer, st_CTS_HEARTBEAT& _value);
int Serialization(char* buffer, st_CTS_MESSAGE& _value);
static int Serialization(char* buffer, st_Header& _value);
int Serialization(char* buffer, st_Msg& _value);
int Serialization(char* buffer, st_STC_ENTER_ZONE& _value);
int Serialization(char* buffer, st_STC_EXIT_ZONE& _value);
int Serialization(char* buffer, st_STC_HEARTBEAT& _value);
int Serialization(char* buffer, st_STC_MESSAGE& _value);
int Serialization(char* buffer, st_String& _value);



int UnSerialization(char* buffer, st_CTS_ENTER_ZONE& _value);
int UnSerialization(char* buffer, st_CTS_EXIT_ZONE& _value);
int UnSerialization(char* buffer, st_CTS_HEARTBEAT& _value);
int UnSerialization(char* buffer, st_CTS_MESSAGE& _value);
static int UnSerialization(char* buffer, st_Header& _value);
int UnSerialization(char* buffer, st_Msg& _value);
int UnSerialization(char* buffer, st_STC_ENTER_ZONE& _value);
int UnSerialization(char* buffer, st_STC_EXIT_ZONE& _value);
int UnSerialization(char* buffer, st_STC_HEARTBEAT& _value);
int UnSerialization(char* buffer, st_STC_MESSAGE& _value);
int UnSerialization(char* buffer, st_String& _value);
