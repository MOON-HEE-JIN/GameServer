#pragma once

namespace GAME 
 {
	enum GAME
	{
		LOOPBACK,					//0
		CONNECTINFO,					//1
		CHANGEZONE,					//2
		ENTERZONE,					//해당 Zone 에 대한 정보 요청
		LEAVEZONE,					//4
		CREATECHAR,					//5
		MOVESTART,					//6
		MOVESTOP,					//7
		CHANGEINGZONE,					//8
	};
};

namespace OBSERVER 
 {
	enum OBSERVER
	{
		CONNET_OBSERVER,					//100001
	};
};

