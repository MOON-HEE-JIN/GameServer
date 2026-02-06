#pragma once
#include <string>

#define ProcThreadCnt 3

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
