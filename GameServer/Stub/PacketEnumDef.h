#pragma once

namespace GAME 
 {
	enum GAME
	{
		LOOPBACK,					//0
		CONNECTINFO,					//1
		CHANGEZONE,					//2
		AOI_IN_PLAYER,					//AOI 안에 들어온 Player
		AOI_IN_PLAYERS,					//AOI 안에 들어온 Players 여러 개
		AOI_OUT_PLAYER,					//AOI 밖으로 나간 Player
		AOI_OUT_PLAYERS,					//AOI 밖으로 나간 Players 여러 개
		MOVESTART,					//7
		MOVESTOP,					//8
		CHANGEINGZONE,					//9
		TELEPORT,					//10
		OTHERMOVESTART,					//다른 객체 움직임 시작
	};
};

namespace OBSERVER 
 {
	enum OBSERVER
	{
		CONNET_OBSERVER = 1000001,			//100001
	};
};

