#pragma once
#include <string>

<<<<<<< Updated upstream
#define ProcThreadCnt 3
=======
#define ProcThreadCnt 3

typedef struct st_Log
{
	std::string filePath;
	std::string log;

	st_Log() : filePath(""), log("") {};
	st_Log(const std::string& _filePath, const std::string& _log)
		: filePath(_filePath), log(_log) {
	};
}LOG_JOB;
>>>>>>> Stashed changes
