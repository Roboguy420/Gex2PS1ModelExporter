#include <Gex2PS1ModelExporter.h>

#include <filesystem>

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

	ifstreamoffset reader(inputFile, std::ifstream::binary);
	reader.exceptions(ifstreamoffset::eofbit);

	if (!reader)
	{
		return 3;
	}

	unsigned int bitshift;
	reader.read((char*)&bitshift, sizeof(bitshift));
	bitshift = ((bitshift >> 9) << 11) + 0x800;
	reader.superOffset = bitshift;

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

	return 0;
}