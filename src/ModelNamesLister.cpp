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
#include "Constants.h"

#include <filesystem>

int listNames(std::ifstream& reader, unsigned int modelsAddressesStart)
{
	reader.seekg(modelsAddressesStart, reader.beg);

	int nameIterator = 1;
	while (true)
	{
		unsigned int specificObjectAddress;
		reader.read((char*)&specificObjectAddress, sizeof(specificObjectAddress));

		if (specificObjectAddress == modelsAddressesStart)
		{
			break;
		}

		long int nextPos = reader.tellg();

		if (nameIterator == 8192)
			break;

		reader.seekg(specificObjectAddress + 0x24, reader.beg);
		unsigned int objNameAddr;
		reader.read((char*)&objNameAddr, sizeof(objNameAddr));
		reader.seekg(objNameAddr, reader.beg);
		std::string objName;
		for (int i = 0; i < 8; i++)
		{
			char objNameChar;
			reader.read((char*)&objNameChar, 1);
			objName += objNameChar;
		}

		std::cout << nameIterator << ": " << objName << std::endl;

		nameIterator++;

		reader.seekg(nextPos, reader.beg);
	}

	std::cout << "Exit Code 0: Successful listing with no errors" << std::endl;
	return 0;
}