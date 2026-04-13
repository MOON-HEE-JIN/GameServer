#include "CGMSession.h"

#include <sstream>
#include <cstring>

#include "CNetServer.h"
#include "../Log/CLog.h"

CGMSession::CGMSession(SOCKET sock)
    : m_sock(sock)
{
}

CGMSession::~CGMSession()
{
    if (m_sock != INVALID_SOCKET)
    {
        closesocket(m_sock);
        m_sock = INVALID_SOCKET;
    }
}

void CGMSession::Run()
{
    char recvBuffer[1024] = {};
    std::string pending;

    SendResponse("OK GM SESSION CONNECTED\n");

    while (g_Net.GetRun())
    {
        const int recvSize = recv(m_sock, recvBuffer, static_cast<int>(sizeof(recvBuffer)), 0);
        if (recvSize <= 0)
        {
            break;
        }

        pending.append(recvBuffer, recvSize);

        size_t nextPos = std::string::npos;
        while ((nextPos = pending.find('\n')) != std::string::npos)
        {
            std::string line = pending.substr(0, nextPos);
            pending.erase(0, nextPos + 1);

            if (!line.empty() && line.back() == '\r')
            {
                line.pop_back();
            }

            if (!ProcessCommand(line))
            {
                return;
            }
        }
    }
}

bool CGMSession::ProcessCommand(const std::string& commandLine)
{
    std::stringstream ss(commandLine);
    std::string command;
    ss >> command;

    if (command.empty())
    {
        return SendResponse("ERR EMPTY COMMAND\n");
    }

    if (command == "HELP")
    {
        return SendResponse("OK COMMANDS: HELP, SHUTDOWN, KICK_PLAYER <playerHandle>, KICK_SESSION <sessionHandle>, QUIT\n");
    }

    if (command == "SHUTDOWN")
    {
        //CNetServer::ServerShutDown();
        return true;
    }
    if (command == "QUIT")
    {
        SendResponse("OK BYE\n");
        return false;
    }

    return SendResponse("ERR UNKNOWN COMMAND\n");
}

bool CGMSession::SendResponse(const char* text)
{
    if (m_sock == INVALID_SOCKET)
    {
        return false;
    }

    const int len = static_cast<int>(strlen(text));
    return send(m_sock, text, len, 0) == len;
}
