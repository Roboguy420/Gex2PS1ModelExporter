/*  Copyright (C) 2023  Roboguy420

    Full copyright notice in Gex2PS1ModelExporter.cpp  */

#include "Gex2PS1SharedFunctions.h"

#include <filesystem>
#include <iostream>

int main(int argc, char* argv[])
{
	if (argc < 2)
	{
		return 1;
		//Needs at least the input file to work
	}
	std::string inputFile = argv[1];

	if (!std::filesystem::exists(inputFile))
	{
		//Input file doesn't exist
		return 2;
	}

	std::ifstream tempReader(inputFile, std::ifstream::binary);
	tempReader.exceptions(std::ifstream::eofbit);

	if (!tempReader)
	{
		return 3;
	}

	unsigned int bitshift;
	tempReader.read((char*)&bitshift, sizeof(bitshift));
	bitshift = ((bitshift >> 9) << 11) + 0x800;
	tempReader.seekg(0, tempReader.end);
	size_t filesize = tempReader.tellg();
	tempReader.seekg(bitshift, tempReader.beg);
	
	std::ofstream tempWriter("Gex2PS1ModelExporterTempfile.drm", std::ifstream::binary);

	while (tempReader.tellg() < filesize)
	{
		unsigned char data;
		tempReader.read((char*)&data, sizeof(data));
		tempWriter << data;
	}
	tempWriter.close();
	std::ifstream reader("Gex2PS1ModelExporterTempfile.drm", std::ifstream::binary);

	unsigned int modelsAddressesStart;
	reader.seekg(0x3C, reader.beg);
	reader.read((char*)&modelsAddressesStart, sizeof(modelsAddressesStart));
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

	reader.close();
	std::remove("Gex2PS1ModelExporterTempfile.drm");

	return 0;
}