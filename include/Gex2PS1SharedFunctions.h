#pragma once

#include <iostream>
#include <fstream>

#ifdef _WIN32
	#include <direct.h>
	#include <Windows.h>
#else
	#include <unistd.h>
#endif

inline char directorySeparator()
{
#ifdef _WIN32
	return '\\';
#else
	return '/';
#endif
}