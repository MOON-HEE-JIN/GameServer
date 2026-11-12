#pragma once

namespace Rpc::ChatGame
{

namespace ERROR_CODE 
 {
	enum ERROR_CODE
	{
		NOT_ERROR,					//
		LOGIN_CONNECT,					//로그인 실패
	};
};

namespace OBJECT 
 {
	enum OBJECT
	{
		PLAYER = 111,			//
		MONSTER = 222,			//
		DROPITEM = 333,			//
	};
};

} // namespace Rpc::ChatGame
