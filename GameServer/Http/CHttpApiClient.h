#pragma once

#include <curl/curl.h>
#include <nlohmann/json.hpp>
#include <string>

#define HttpClientType 0

struct st_RegisterServerInfo
{
	std::string Name;
	int  Type;
	std::string IP;
	int  Port;
};

struct st_Heartbeat
{
	std::string Name;
	int Type;
	int Connect;
};

struct st_ApiResult
{
	bool bSuccess = false;
	long httpCode = 0;
	std::string body;
	std::string errror;

	bool IsSuccess() const { return bSuccess && httpCode == 200; }
};

class CApiClient
{
public:
	CApiClient(const std::string url) : baseUrl(url) { curl_global_init(CURL_GLOBAL_DEFAULT); }
	~CApiClient();
private:
	std::string baseUrl;
	bool m_bConnect;

	static size_t WriteResponseCallback(char* contents, size_t size, size_t nmemb, void* userp) noexcept;
	void HttpSend(const std::string _url, const std::string _jsonString, st_ApiResult& result);

public:
	void Init();
	void Start();
	void Wait();
	st_ApiResult RegisterServer(const st_RegisterServerInfo& info);
	st_ApiResult HeartbeatServer(const st_Heartbeat& info);

	bool GetConnect() { return m_bConnect; }
};

static HANDLE s_hThread;
static HANDLE s_hExit;
static unsigned __stdcall LoginServerAPIThread(void* arg);