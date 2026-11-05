#include "CHttpApiClient.h"
#include "../NetWork/CNetServer.h"
#include "../Log/CLog.h"

#include <memory>
#include <process.h>

CApiClient::~CApiClient()
{
    curl_global_cleanup();
}

st_ApiResult CApiClient::RegisterServer(const st_RegisterServerInfo& info)
{
    st_ApiResult result{};
    
    const std::string url = "/api/serverconnection/register";
    const nlohmann::json jsonData = {
        {"serverId", info.Name},
        {"type", info.Type},
        {"address", info.IP},
        {"port", info.Port}
    };
    const std::string jsonString = jsonData.dump();

	HttpSend(url, jsonData.dump(), result);
 
    if(result.bSuccess)
		m_bConnect = true;

    return result;
}

st_ApiResult CApiClient::HeartbeatServer(const st_Heartbeat& info)
{
    st_ApiResult result{};

	const std::string url = "/api/serverconnection/heartbeat";
	const nlohmann::json jsonData = {
		{"serverId", info.Name},
		{"type", info.Type},
		{"connect", info.Connect}
	};

    HttpSend(url, jsonData.dump(), result);
    
    return result;
}

size_t CApiClient::WriteResponseCallback(
    char* contents, size_t size, size_t nmemb, void* userp) noexcept
{
    const size_t totalSize = size * nmemb;
    auto* response = static_cast<std::string*>(userp);
    try
    {
        response->append(contents, totalSize);
        return totalSize;
    }
    catch (...)
    {
        return 0;
    }
}

void CApiClient::HttpSend(const std::string _url, const std::string _jsonString, st_ApiResult& result)
{
    using CurlHandle = std::unique_ptr<CURL, decltype(&curl_easy_cleanup)>;
    CurlHandle curl(curl_easy_init(), &curl_easy_cleanup);
    if (!curl)
    {
        result.errror = "Failed to initialize CURL";
        return ;
    }

    using HeaderList = std::unique_ptr<curl_slist, decltype(&curl_slist_free_all)>;
    HeaderList headers(
        curl_slist_append(nullptr, "Content-Type: application/json"),
        &curl_slist_free_all);
    if (!headers)
    {
        result.errror = "Failed to allocate HTTP headers";
        return ;
    }

    const std::string url = baseUrl + _url;
	const std::string jsonString = _jsonString;

    curl_easy_setopt(curl.get(), CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl.get(), CURLOPT_POST, 1L);
    curl_easy_setopt(curl.get(), CURLOPT_HTTPHEADER, headers.get());
    curl_easy_setopt(curl.get(), CURLOPT_POSTFIELDS, jsonString.c_str());
    curl_easy_setopt(curl.get(), CURLOPT_POSTFIELDSIZE_LARGE,
        static_cast<curl_off_t>(jsonString.size()));
    curl_easy_setopt(curl.get(), CURLOPT_CONNECTTIMEOUT_MS, 3000L);
    curl_easy_setopt(curl.get(), CURLOPT_TIMEOUT_MS, 5000L);
    curl_easy_setopt(curl.get(), CURLOPT_NOSIGNAL, 1L);
    curl_easy_setopt(curl.get(), CURLOPT_WRITEFUNCTION, WriteResponseCallback);
    curl_easy_setopt(curl.get(), CURLOPT_WRITEDATA, &result.body);

    const CURLcode res = curl_easy_perform(curl.get());
    if (res != CURLE_OK)
    {
        result.errror = curl_easy_strerror(res);
        return;
    }

    curl_easy_getinfo(curl.get(), CURLINFO_RESPONSE_CODE, &result.httpCode);
    result.bSuccess = true;
    if (!result.IsSuccess())
    {
        result.errror = "HTTP " + std::to_string(result.httpCode);
    }
}

void CApiClient::Init()
{
    
}

void CApiClient::Start()
{
    const st_ApiResult registration =
        RegisterServer({ g_Net.GetServerName(), 0, g_Net.GetServerIP(), g_Net.GetPort() });
    if (!registration.IsSuccess())
    {
        std::fprintf(stderr, "LoginServer registration failed (HTTP %ld): %s\n%s\n",
            registration.httpCode, registration.errror.c_str(), registration.body.c_str());
        return ;
    }
    else
        std::fprintf(stdout, "LoginServer registration succeeded (HTTP %ld)\n", registration.httpCode);

	s_hThread = (HANDLE)_beginthreadex(NULL, 0, LoginServerAPIThread, this, 0, NULL);
}

void CApiClient::Wait()
{
	if (s_hThread)
	{
		WaitForSingleObject(s_hThread, INFINITE);
		CloseHandle(s_hThread);
		s_hThread = nullptr;
	}
}

unsigned __stdcall LoginServerAPIThread(void* arg)
{
	CApiClient* pClient = static_cast<CApiClient*>(arg);

    if (!g_Net.GetRun())
        exit(1);

	s_hExit = CreateEvent(NULL, TRUE, FALSE, NULL);
	if (s_hExit == NULL)
	{
		g_LogServer.ELog("Create LoginServerAPIThread exit event failed");
		return 0;
	}

    int ret = 0;
    int debugCount = 0;
    while (g_Net.GetRun())
    {
		ret = WaitForSingleObject(s_hExit, 5000);

		// 종료 이벤트
		if (ret == WAIT_OBJECT_0)
			break;

		if (!pClient->GetConnect())
		{
			g_LogServer.ELog("LoginServer is not connected to API server.");
			pClient->RegisterServer({ g_Net.GetServerName(), 0, g_Net.GetServerIP(), g_Net.GetPort() });
            continue;
		}

		st_Heartbeat heartbeat;
		
        heartbeat.Name = g_Net.GetServerName();
		heartbeat.Type = 0;
		heartbeat.Connect = g_Net.GetPlayerCount() + (++debugCount);
		
        const st_ApiResult result = pClient->HeartbeatServer(heartbeat);
		
        if (!result.IsSuccess())
		{
			g_LogServer.ELog("LoginServer Heartbeat failed (HTTP %ld): %s\n%s\n",
				result.httpCode, result.errror.c_str(), result.body.c_str());
		}
        
    }
    return 0;
}
