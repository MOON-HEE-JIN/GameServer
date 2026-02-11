#pragma once

#include <WinSock2.h>
#include <string>

class CGMSession
{
public:
    CGMSession(SOCKET sock);
    ~CGMSession();

    void Run();

private:
    SOCKET m_sock;

    bool ProcessCommand(const std::string& commandLine);
    bool SendResponse(const char* text);
};