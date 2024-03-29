/*  Gex2PS1ModelExporter: Command line program for exporting Gex 2 PS1 models
    Copyright (C) 2023  Roboguy420

    Gex2PS1ModelExporter is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    Gex2PS1ModelExporter is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with Gex2PS1ModelExporter.  If not, see <https://www.gnu.org/licenses/>.  */

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

inline std::string tempDirectory()
{
#ifdef _WIN32
    if (std::getenv("TEMP") != NULL)
        return std::getenv("TEMP");
    else
        return ".";
#else
    return "/tmp";
#endif
}

float rgbToLinearRgb(unsigned char colour);

std::string divideByAPowerOfTen(int inputNumber, unsigned int powerOfTen);

int stringToInt(std::string inputString, int failValue);

std::string getFileNameWithoutExtension(std::string fileName, bool includePath);

unsigned int oneOrZero(auto number, auto threshold);

unsigned int getMostDifferentIndexOfThree(auto element1, auto element2, auto element3);

unsigned int getMinOrMaxIndexOfThree(auto element1, auto element2, auto element3, bool min);