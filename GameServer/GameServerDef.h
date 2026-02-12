#pragma once
#include <string>

#define ProcThreadCnt 3

enum eZONESTATUS
{
	NONE,
	STABLE,		// 완료
	ENTER,		// 들어가는 중
	LEAVE,		// 나가는 중
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

typedef struct st_ZoneJob
{
	unsigned int time;			// Job 입력 시간
	eZONESTATUS type;			// Job Type
	int handle;					// Player Handle
	int toZone;					// 목표 Zone
	int fromZone;				// 시작 Zone
	bool ack;					// ack 신호
	bool ret;					// ack 에 대한 성공 여부
	st_ZoneJob() : time(0), type(eZONESTATUS::NONE), handle(0), toZone(0), fromZone(0), ack(false), ret(false) {};
	st_ZoneJob(unsigned int _time, eZONESTATUS _type, int _handle, int _to, int _from, bool _ack, bool _ret)
		: time(_time), type(_type), handle(_handle), toZone(_to), fromZone(_from), ack(_ack), ret(_ret) { }

	st_ZoneJob(const st_ZoneJob&) = default;
	void operator()(unsigned int _time, eZONESTATUS _type, int _handle, int _to, int _from, bool _ack, bool _ret)
	{
		time = _time;
		type = _type;
		handle = _handle;
		toZone = _to;
		fromZone = _from;
		ack = _ack;
		ret = _ret;
	}
}ZONE_JOB;