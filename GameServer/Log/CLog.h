#pragma once

#include <format>
#include <iostream>
class CLog
{
public:
	template<typename ... Args>
	void Info(std::format_string<Args ...> fmtStr, Args&&... args);
};