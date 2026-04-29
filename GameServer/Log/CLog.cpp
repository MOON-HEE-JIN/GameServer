#include "CLog.h"

#include "../MemoryManager/MemoryManager.h"
#include "../NetWork/CNetServer.h"
#include "../ZoneManager/CZoneManager.h"

#include <chrono>
#include <sstream>
#include <windows.h>
#include <cstdarg>
#include <process.h>

static HANDLE h_hExit;
static HANDLE s_hLogHandle;
CLog::CLog(const char* filePath, const char* comment)
{
	fp = nullptr;
	m_filePath = filePath;
	m_logComment = comment;
}

CLog::~CLog()
{
}

void CLog::DLog(const char* format, ...)
{
#ifdef _DEBUG
	va_list args;
	va_start(args, format);
	std::string log = BuildMessage("DEBUG", format, args);
	va_end(args);

	LOG_JOB job(m_filePath, log);

	g_LogJobQueue.Enqueue({ m_filePath, log });
#endif // _DEBUG
}

void CLog::ILog(const char* format, ...)
{
	va_list args;
	va_start(args, format);
	std::string log = BuildMessage("INFO", format, args);
	va_end(args);

	LOG_JOB job(m_filePath, log);

	g_LogJobQueue.Enqueue({ m_filePath, log });
}

void CLog::ELog(const char* format, ...)
{
	va_list args;
	va_start(args, format);
	std::string log = BuildMessage("ERROR", format, args);
	va_end(args);

	LOG_JOB job(m_filePath, log);

	g_LogJobQueue.Enqueue({ m_filePath, log });
}

std::string CLog::BuildMessage(const char* level, const char* format, va_list args)
{
	auto now = std::chrono::system_clock::now();
	auto nowTime = std::chrono::system_clock::to_time_t(now);

	tm localTime = {};
	localtime_s(&localTime, &nowTime);

	char timeBuffer[32] = {};
	snprintf(timeBuffer, sizeof(timeBuffer), "%04d-%02d-%02d %02d:%02d:%02d",
		localTime.tm_year + 1900,
		localTime.tm_mon + 1,
		localTime.tm_mday,
		localTime.tm_hour,
		localTime.tm_min,
		localTime.tm_sec);

	va_list argsCopy;
	va_copy(argsCopy, args);
	int formattedSize = _vscprintf(format, argsCopy);
	va_end(argsCopy);

	if (formattedSize < 0)
	{
		formattedSize = 0;
	}

	std::string formattedMessage;
	formattedMessage.resize(static_cast<size_t>(formattedSize));
	if (formattedSize > 0)
	{
		vsnprintf(&formattedMessage[0], formattedMessage.size() + 1, format, args);
	}

	std::ostringstream stream;
	stream << timeBuffer << " [" << m_logComment << "::" << level << "] " << formattedMessage << "\n";
	return stream.str();
}


void CreateLogThread()
{
	s_hLogHandle = (HANDLE)_beginthreadex(NULL, 0, LogThread, 0, 0, NULL);
	h_hExit = CreateEvent(NULL, TRUE, FALSE, NULL);
}

void PostMessageLogThreadExit()
{
	SetEvent(h_hExit);
}

void WaitLogThread()
{
	WaitForSingleObject(s_hLogHandle, INFINITE);
}

unsigned __stdcall LogThread(void* arg)
{
	
	int ret = 0;
	while (g_Net.GetRun())
	{
		ret = WaitForSingleObject(h_hExit, 1);

		g_ZoneManager.Log();

		LOG_JOB job;
		while (g_LogJobQueue.TryDequeue(job))
		{
			fputs(job.log.c_str(), stdout);

			FILE* fp;
			fopen_s(&fp, job.filePath.c_str(), "a");
			if (fp == nullptr)
				continue;

			fprintf(fp, job.log.c_str());
			fclose(fp);
		}
	}

	LOG_JOB job;
	while (g_LogJobQueue.TryDequeue(job))
	{
		fputs(job.log.c_str(), stdout);

		FILE* fp;
		fopen_s(&fp, job.filePath.c_str(), "a");
		if (fp == nullptr)
			continue;

		fprintf(fp, job.log.c_str());
		fclose(fp);
	}

	// LogThread 로그 다 찍고 마지막에 제거
	Sleep(1000);

	fputs("=== END THREAD LogThread ===\n", stdout);

	return 0;
}

CLog g_LogTemp("temp.log", "TEMP");
CLog g_LogServer("server.log", "SERVER");
CLog g_LogGame("game.log", "GAME");
CLog g_LogThread("thread.log", "THREAD");
CLog g_LogFile("file.log", "FILE");
CLog g_LogObserver("observer.log", "OBSERVER");