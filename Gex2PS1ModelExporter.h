#pragma once

#include <iostream>
#include <stdio.h>
#include <direct.h>
#include <fstream>
#include <string>
#include <format>

int stringToInt(std::string inputString, int failValue)
{
	for (int i = 0; i < inputString.length(); i++)
	{
		if (!isdigit(inputString[i]))
		{
			return failValue;
		}
	}
	return atoi(inputString.c_str());
}

inline char directorySeparator()
{
#ifdef _WIN32
	return '\\';
#else
	return '/';
#endif
}

std::string getFileNameWithoutExtension(std::string fileName)
{
	//Does not return the path, even if there is one in the argument
	size_t parentDirEnd = fileName.find_last_of(directorySeparator());
	if (parentDirEnd != std::string::npos)
	{
		fileName = fileName.substr(parentDirEnd + 1);
	}
	size_t extensionStart = fileName.find_last_of('.');
	if (extensionStart == std::string::npos)
	{
		return fileName; //Simply returns the filename if there is no extension
	}
	return fileName.substr(0, extensionStart);
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
		inputNumberString = inputNumberString.erase(0, 1);
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
		leftOfDecimal = inputNumberString.erase(inputNumberString.length() - powerOfTen, powerOfTen);
		rightOfDecimal = inputNumberString.erase(0, inputNumberString.length() - powerOfTen);
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

unsigned int oneOrZero(auto number, auto threshold)
{
	//If the number is greather than the threshold, return one
	//If the number is less than the threshold, return zero
	if (number > threshold)
	{
		return 1;
	}
	return 0;
}

unsigned int getMostDifferentIndexOfThree(auto element1, auto element2, auto element3)
{
	if (abs(element1 - element2) >= abs(element2 - element3) && abs(element1 - element3) >= abs(element2 - element3))
	{
		return 0;
	}
	else if (abs(element2 - element1) >= abs(element1 - element3) && abs(element2 - element3) >= abs(element1 - element3))
	{
		return 1;
	}
	else
	{
		return 2;
	}
}

unsigned int getMinOrMaxIndexOfThree(auto element1, auto element2, auto element3, bool min)
{
	if (!min)
	{
		if (element1 >= element2 && element1 >= element3)
		{
			return 0;
		}
		else if (element2 >= element1 && element2 >= element3)
		{
			return 1;
		}
		else
		{
			return 2;
		}
	}
	else
	{
		if (element1 <= element2 && element1 <= element3)
		{
			return 0;
		}
		else if (element2 <= element1 && element2 <= element3)
		{
			return 1;
		}
		else
		{
			return 2;
		}
	}
}

class ifstreamoffset : public std::ifstream
{
	//Pretty much identical to ifstream for the most part
	//However it includes an offset (named superOffset) that can be used to reposition the "start" of the stream
	//E.g. If you have a filestream, but everything before 0x1000 is unneeded, you can make 0x1000 the offset
	//This would allow you to make arbitrary movements from the beginning of the stream without needing to constantly reuse the offset

	public:
		using std::ifstream::ifstream;

		unsigned int superOffset;

		basic_istream& __CLR_OR_THIS_CALL seekg(off_type _Off, ios_base::seekdir _Way, bool offsetUsed = true)
		{
			unsigned int offset = 0;
			if (offsetUsed && _Way == ios_base::beg) { offset = superOffset; }

			ios_base::iostate _State = ios_base::goodbit;
			ios_base::iostate _Oldstate = _Myios::rdstate();
			_Myios::clear(_Oldstate & ~ios_base::eofbit);
			const sentry _Ok(*this, true);

			if (!this->fail())
			{
				_TRY_IO_BEGIN
				if (static_cast<off_type>(_Myios::rdbuf()->pubseekoff(_Off + offset, _Way, ios_base::in)) == -1)
				{
					_State |= ios_base::failbit;
				}
				_CATCH_IO_END
			}

			_Myios::setstate(_State);
			return *this;
		}

		pos_type __CLR_OR_THIS_CALL tellg(bool offsetUsed = true)
		{
			unsigned int offset = 0;
			if (offsetUsed) { offset = superOffset; }

			const sentry _Ok(*this, true);

			if (!this->fail()) {
				_TRY_IO_BEGIN
					return (int)_Myios::rdbuf()->pubseekoff(0, ios_base::cur, ios_base::in) - offset;
				_CATCH_IO_END
			}

			return pos_type(-1);
		}
};