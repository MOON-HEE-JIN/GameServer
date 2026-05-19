#pragma once

enum eMOVESTATE
{
	STOPPED,
	MOVEING,
	RUNNING,
};

enum eZONESTATUS
{
	NONE,
	STABLE,		// 완료
	ENTER,		// 들어가는 중
	LEAVE,		// 나가는 중
	RELEASE,	// 삭제
};

enum EGRID_ADD_TYPE
{
	GRID_ENTER,
	GRID_LEAVE,
	ADD_TELEPORT,
	SUB,
	END = SUB,
};
