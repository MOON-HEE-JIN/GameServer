#include "CPlayer.h"
#include "NetWork/CNetServer.h"
#include "ZoneManager/CZoneManager.h"
#include "Log/CLog.h"

void CPlayer::Init(SESSION_HANDLE sessionID, int handle, int Channel, int Zone)
{
	m_iVarRef.store(0);
	
	m_nEntityType = eENTITY_TYPE::ENTITY_PLAYER;
	m_SessionHandle.store(sessionID);
	m_PlayerHandle = handle;
	m_iChannel = Channel;
	m_OwnerZone = Zone;

	m_bRelease.store(false);
	ResetVisiblePlayers();

	m_iGridID.store(-1, std::memory_order_relaxed);
	m_iPendingGridID.store(-1, std::memory_order_relaxed);

	AddVarRef();
}

void CPlayer::Clear()
{
	Reset();

	m_SessionHandle.store(SESSION_HANDLE(-1, 0));
	m_PlayerHandle = -1;
	m_iChannel = 0;
	m_OwnerZone = 0;

	m_iGridID.store(-1, std::memory_order_relaxed);
	m_iPendingGridID.store(-1, std::memory_order_relaxed);

	m_bRelease.store(false);
	ResetVisiblePlayers();
}

void CPlayer::AddVarRef()
{
	m_iVarRef.fetch_add(1);
	AddRef();
}

void CPlayer::ReleaseVarRef()
{
	int ref = m_iVarRef.fetch_sub(1);
	if (ref <= 0)
	{
		m_iVarRef.fetch_add(1);
		g_LogRef.ELog("Player Handle %d ReleaseVarRef Error", GetID());
		return;
	}

	ReleaseRef();
}

void CPlayer::OnRelease()
{
	int key = GetID();
	Clear();
	g_Net.AddPlayerHandle(key);
}

void CPlayer::SetRelease()
{
	m_bRelease.store(true);
	SessionHandleClear();
}

void CPlayer::SendPacket(CPacket* pPacket)
{
	TrySend(m_SessionHandle.load(), pPacket);
}

void CPlayer::BroadCast(CPacket* pPacket)
{
	g_ZoneManager.SendZone(GetChannel(), GetZoneID(), pPacket, GetTilePos(), this);
}

void CPlayer::ResetVisiblePlayers()
{
	std::lock_guard<std::mutex> guard(m_AoiLock);
	m_setVisiblePlayerIDs.clear();
}

void CPlayer::ApplyAoiDelta(
	const std::vector<CEntity*>& previousOnly,
	const std::vector<CEntity*>& currentOnly)
{
	std::lock_guard<std::mutex> guard(m_AoiLock);

	st_STC_AoiOutPlayers outPlayers{};
	for (CEntity* pEntity : previousOnly)
	{
		if (pEntity == nullptr || pEntity == this)
			continue;

		const int visibleID = pEntity->GetID();
		if (m_setVisiblePlayerIDs.erase(visibleID) == 0)
			continue;

		outPlayers.info[outPlayers.Loop1++].ID = visibleID;
		if (outPlayers.Loop1 == 50)
		{
			SendPacket(outPlayers);
			outPlayers = {};
		}
	}

	if (outPlayers.Loop1 > 0)
		SendPacket(outPlayers);

	st_STC_AoiInPlayers inPlayers{};
	st_STC_AoiInPlayerMoves inMoves{};
	for (CEntity* pEntity : currentOnly)
	{
		if (pEntity == nullptr || pEntity == this ||
			pEntity->GetEntityType() != eENTITY_TYPE::ENTITY_PLAYER)
			continue;

		CPlayer* pVisiblePlayer = static_cast<CPlayer*>(pEntity);
		if (pVisiblePlayer->GetRelease())
			continue;

		const int visibleID = pEntity->GetID();
		if (!m_setVisiblePlayerIDs.insert(visibleID).second)
			continue;

		st_PlayerInfo& info = inPlayers.info[inPlayers.Loop1++];
		info.ID = visibleID;
		info.pos = pEntity->GetPosition();
		info.speed = pEntity->GetMoveSpeed();

		if (pEntity->GetMoveState() != eMOVESTATE::STOPPED)
		{
			st_PlayerOtherMove& move = inMoves.move[inMoves.Loop1++];
			move.ID = visibleID;
			move.pos = pEntity->GetPosition();
			move.dir = pEntity->GetDirVector();
		}

		if (inPlayers.Loop1 == 50)
		{
			SendPacket(inPlayers);
			inPlayers = {};
			if (inMoves.Loop1 > 0)
			{
				SendPacket(inMoves);
				inMoves = {};
			}
		}
	}

	if (inPlayers.Loop1 > 0)
		SendPacket(inPlayers);
	if (inMoves.Loop1 > 0)
		SendPacket(inMoves);
}

void CPlayer::NotifyAoiEnter(CEntity* pEntity)
{
	if (pEntity == nullptr || pEntity == this ||
		pEntity->GetEntityType() != eENTITY_TYPE::ENTITY_PLAYER)
		return;

	CPlayer* pVisiblePlayer = static_cast<CPlayer*>(pEntity);
	if (pVisiblePlayer->GetRelease())
		return;

	std::lock_guard<std::mutex> guard(m_AoiLock);
	const int visibleID = pEntity->GetID();
	if (!m_setVisiblePlayerIDs.insert(visibleID).second)
		return;

	st_STC_AoiInPlayer inPlayer{};
	inPlayer.info.ID = visibleID;
	inPlayer.info.pos = pEntity->GetPosition();
	inPlayer.info.speed = pEntity->GetMoveSpeed();
	SendPacket(inPlayer);

	if (pEntity->GetMoveState() != eMOVESTATE::STOPPED)
	{
		st_STC_OtherMoveStart move{};
		move.type = pEntity->GetType();
		move.ID = visibleID;
		move.pos = pEntity->GetPosition();
		move.dir = pEntity->GetDirVector();
		SendPacket(move);
	}
}

void CPlayer::NotifyAoiLeave(int entityID)
{
	if (entityID == GetID())
		return;

	std::lock_guard<std::mutex> guard(m_AoiLock);
	if (m_setVisiblePlayerIDs.erase(entityID) == 0)
		return;

	st_STC_AoiOutPlayer outPlayer{};
	outPlayer.ID = entityID;
	SendPacket(outPlayer);
}

#ifdef __DEBUG__
bool CPlayer::DebugCheckVisiblePlayers(
	const std::unordered_set<int>& expectedIDs,
	int& missingCount,
	int& staleCount) const
{
	std::lock_guard<std::mutex> guard(m_AoiLock);
	missingCount = 0;
	staleCount = 0;
	for (int expectedID : expectedIDs)
	{
		if (!m_setVisiblePlayerIDs.contains(expectedID))
			++missingCount;
	}
	for (int visibleID : m_setVisiblePlayerIDs)
	{
		if (!expectedIDs.contains(visibleID))
			++staleCount;
	}
	return missingCount == 0 && staleCount == 0;
}
#endif

bool CPlayer::Teleport(st_Vector3F pos)
{
	st_Vector3F originPos = m_stPosition;

	if (!((CZoneBasic*)m_pZone)->Teleport(this, pos))
	{
		m_stPosition = originPos;
		return false;
	}
	return true;
}


