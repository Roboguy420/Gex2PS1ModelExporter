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

#include "Constants.h"
#include "Globals.h"

#include <iostream>

int listNames(unsigned int modelsAddressesStart)
{
	g_reader.seekg(modelsAddressesStart, g_reader.beg);

	int nameIterator = 1;
	while (true)
	{
		unsigned int specificObjectAddress;
		g_reader.read((char*)&specificObjectAddress, sizeof(specificObjectAddress));

		if (specificObjectAddress == modelsAddressesStart)
		{
			break;
		}

		long int nextPos = g_reader.tellg();

		if (nameIterator == 8192)
			break;

		g_reader.seekg(specificObjectAddress + 0x24, g_reader.beg);
		unsigned int objNameAddr;
		g_reader.read((char*)&objNameAddr, sizeof(objNameAddr));
		g_reader.seekg(objNameAddr, g_reader.beg);
		std::string objName;
		for (int i = 0; i < 8; i++)
		{
			char objNameChar;
			g_reader.read((char*)&objNameChar, 1);
			objName += objNameChar;
		}

		std::cout << nameIterator << ": " << objName << std::endl;

		nameIterator++;

		g_reader.seekg(nextPos, g_reader.beg);
	}

	std::cout << "Exit Code 0: Successful listing with no errors" << std::endl;
	return 0;
}
