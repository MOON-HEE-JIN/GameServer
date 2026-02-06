#pragma once

#include <string>

class CLog
{
public:
	CLog(const char* filePath, const char* comment);
	~CLog();

private:
	FILE* fp;
	const char* m_filePath;
	const char* m_logComment;
public:
	void DLog(const char* format, ...);
	void ILog(const char* format, ...);
	void ELog(const char* format, ...);
private:
	std::string BuildMessage(const char* level, const char* format, va_list args);
};

void CreateLogThread();
void WaitLogThread();
static unsigned __stdcall LogThread(void* arg);

extern CLog g_LogTemp;
extern CLog g_LogServer;
extern CLog g_LogGame;
