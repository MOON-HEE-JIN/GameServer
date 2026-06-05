#pragma once
#include "GameServerEnumDef.h"
#include <string>

#define ProcLoginThreadCnt 1
#define ProcMainThreadCnt 1
#define ProcSubThreadCnt 2
#define ProcThreadCnt ProcLoginThreadCnt + ProcMainThreadCnt + ProcSubThreadCnt

#define AOI_VIEW_COUNT 2

#define POSITION_TOLERANCE 5
#define FIXED_DELTA 0.01667f		// fps 60
#define MAX_FRAME_LOOP_COUNT 6

enum EIndexType
{
	VECTOR_INDEX_ZONE,
	VECTOR_INDEX_GRID,
	VECTOR_INDEX_TILE,
	VECTOR_INDEX_MOVE,
	VECTOR_INDEX_END
};

typedef struct st_Log
{
	std::string filePath;
	std::string log;

	st_Log() : filePath(""), log("") {};
	st_Log(const std::string& _filePath, const std::string& _log)
		: filePath(_filePath), log(_log) {
	};
	st_Log& operator=(st_Log&& req) noexcept
	{
		if (this != &req)
		{
			filePath = req.filePath;
			log = req.log;
		}
		return *this;
	}
}LOG_JOB;

typedef struct st_ChangeZoneJob
{
	unsigned int time;			// Job 입력 시간
	eZONESTATUS type;			// Job Type
	int handle;					// Player Handle
	int toID;					// 목표 ID
	int toZone;					// 목표 Zone
	int fromID;					// 시작 ID
	int fromZone;				// 시작 Zone
	bool ack;					// ack 신호
	bool ret;					// ack 에 대한 성공 여부
	st_ChangeZoneJob() : time(0), type(eZONESTATUS::NONE), handle(0), toID(0), toZone(0), fromID(0), fromZone(0), ack(false), ret(false) {};
	st_ChangeZoneJob(unsigned int _time, eZONESTATUS _type, int _handle, int _toID, int _to, int _fromID, int _from, bool _ack, bool _ret)
		: time(_time), type(_type), handle(_handle), toID(_toID), toZone(_to), fromID(_fromID), fromZone(_from), ack(_ack), ret(_ret) { }

	st_ChangeZoneJob(const st_ChangeZoneJob&) = default;
	void operator()(unsigned int _time, eZONESTATUS _type, int _handle, int _toID, int _to, int _fromID, int _from, bool _ack, bool _ret)
	{
		time = _time;
		type = _type;
		handle = _handle;
		toID = _toID;
		toZone = _to;
		fromID = _fromID;
		fromZone = _from;
		ack = _ack;
		ret = _ret;
	}
}ZONE_CHANGE_JOB;

typedef struct st_GridPos
{
	int X;
	int Z;

	st_GridPos() = default;
	st_GridPos(int _X, int _Z) : X(_X), Z(_Z) {};

	bool operator==(const st_GridPos& other) const
	{
		return (X == other.X && Z == other.Z);
	}

	st_GridPos operator-(const st_GridPos& other) const
	{
		return st_GridPos(X - other.X, Z - other.Z);
	}

}COORDINATE;
