#pragma once
#include "../CUtill/CPacket.h"
#include <atomic>
#include <basetsd.h>

#define OVERLAP_RUN_THREAD 3
#define MAX_CONNECT_COUNT 100000

static constexpr ULONG_PTR KEY_SHUTDOWN_WAKE = 3;

typedef struct st_ObjectHandle 
{
	int Handle;
	int Gen;
	st_ObjectHandle() : Handle(-1), Gen(0) {};
	st_ObjectHandle(int _id, int _gen) : Handle(_id), Gen(_gen) {};
	bool operator==(const st_ObjectHandle& rhs) const
	{
		return Handle == rhs.Handle && Gen == rhs.Gen;
	}
	bool operator!=(const st_ObjectHandle& rhs) const
	{
		return !(*this == rhs);
	}

} SESSION_HANDLE;

typedef struct st_PACKET_JOB
{
	SESSION_HANDLE SessionHandle;
	int PlayerHandle;	// 해당 패킷 처리를 해야할 Player ID
	int type;
	CPacket packet;

	st_PACKET_JOB() : SessionHandle(), PlayerHandle(-1), type(0) {};
	st_PACKET_JOB(SESSION_HANDLE _sessionHandle, int _id, int _type, CPacket& _packet)
		: SessionHandle(_sessionHandle), PlayerHandle(_id), type(_type), packet(_packet) {};
	st_PACKET_JOB& operator=(st_PACKET_JOB&& job) noexcept
	{
		if (this != &job)
		{
			SessionHandle = job.SessionHandle;
			PlayerHandle = job.PlayerHandle;
			type = job.type;
			//packet = std::move(job.packet);
			packet = job.packet;
		}
		return *this;
	}

	st_PACKET_JOB(const st_PACKET_JOB&) = default;
	st_PACKET_JOB& operator=(st_PACKET_JOB& job)
	{
		SessionHandle = job.SessionHandle;
		PlayerHandle = job.PlayerHandle;
		type = job.type;
		packet = job.packet;
		return *this;
	}


}PROC_MSG;


typedef struct st_SendReq
{
	SESSION_HANDLE SessionHandle;
	int type;
	CPacket packet;

	st_SendReq() : SessionHandle(), type(0) {};
	st_SendReq(SESSION_HANDLE _handle, int _type, CPacket _packet)
		: SessionHandle(_handle), type(_type) {
		packet = _packet;
	};
	st_SendReq& operator=(st_SendReq&& req) noexcept
	{
		if (this != &req)
		{
			SessionHandle = req.SessionHandle;
			type = req.type;
			//packet = std::move(req.packet);
			packet = req.packet;
		}
		return *this;
	}
} SEND_REQ;

