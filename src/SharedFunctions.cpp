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

#include "SharedFunctions.h"

#include <string>
#include <format>

float rgbToLinearRgb(unsigned char colour)
{
	if (colour <= 10)
		return (float)(colour / 255.0f / 12.92f);
	else
		return (float)(std::pow(((colour / 255.0f) + 0.055f) / 1.055f, 2.4));
}

std::string divideByAPowerOfTen(int inputNumber, unsigned int powerOfTen)
{
	//Workaround for those pesky floating point rounding errors whenever you divide a number by power of 10
	
	//powerOfTen is the index of whichever power of ten you're dividing by
	//E.g. if you wanted to divide by 1000, you would use 3 for powerOfTen, as 10^3 = 1000
	
	if (powerOfTen == 0)
	{
		return std::to_string(inputNumber);
	}

	std::string leftOfDecimal;
	std::string rightOfDecimal;

	std::string inputNumberString = std::to_string(inputNumber);
	bool negative = false;
	if (inputNumberString[0] == '-')
	{
		inputNumberString.erase(0, 1);
		negative = true;
	}
	
	if (inputNumberString.length() <= powerOfTen)
	{
		leftOfDecimal = "0";
		rightOfDecimal = inputNumberString;
		for (int i = 0; i < powerOfTen - inputNumberString.length(); i++)
		{
			rightOfDecimal = "0" + rightOfDecimal;
		}
	}
	else
	{
		std::string inputNumberStringLeft = inputNumberString;
		std::string inputNumberStringRight = inputNumberString;
		leftOfDecimal = inputNumberStringLeft.erase(inputNumberString.length() - powerOfTen, powerOfTen);
		rightOfDecimal = inputNumberStringRight.erase(0, inputNumberString.length() - powerOfTen);
	}
	
	if (negative)
	{
		leftOfDecimal = '-' + leftOfDecimal;
	}

	int indexOfLastNonZero = -1;

	for (int c = 0; c < rightOfDecimal.length(); c++)
	{
		if (rightOfDecimal[c] != '0')
		{
			indexOfLastNonZero = c;
		}
	}

	if (indexOfLastNonZero < rightOfDecimal.length() - 1 && indexOfLastNonZero != -1)
	{
		rightOfDecimal = rightOfDecimal.erase(indexOfLastNonZero + 1, std::string::npos);
	}

	if (rightOfDecimal == "")
	{
		rightOfDecimal = "0";
	}

	return std::format("{}.{}", leftOfDecimal, rightOfDecimal);
}