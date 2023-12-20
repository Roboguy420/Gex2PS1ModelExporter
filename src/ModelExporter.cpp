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

#define NOMINMAX

#include "SharedFunctions.h"
#include "ModelExporter.h"
#include "ModelNamesLister.h"
#include "TextureExporter.h"
#include "VerticesInterpreter.h"
#include "PolygonsInterpreter.h"
#include "XMLExport.h"
#include "Constants.h"

#include <format>
#include <filesystem>
#include <vector>
#include <math.h>
#include <getopt.h>

int main(int argc, char* argv[])
{
	std::string inputFile;
	std::string outputFolder = std::filesystem::current_path().string();

	// Selected export -1 = everything
	// Selected export 0 = level geometry
	// Selected export >0 = other object models
	int selectedModelExport = -1;

	bool listNamesBool = false;

	static struct option long_options[] =
	{
		{"out", required_argument, 0, 'o'},
		{"index", required_argument, 0, 'i'},
		{"list", no_argument, 0, 'l'},
		{0, 0, 0, 0}
	};

	int opt;
	while ((opt = getopt_long(argc, argv, "o:i:l", long_options, NULL)) != -1)
	{
		switch (opt)
		{
			case 'o':
				outputFolder = optarg;
				break;
			case 'i':
				if ((selectedModelExport = stringToInt(optarg, -2)) < -1)
				{
					std::cerr << "Usage: gex2ps1modelexporter file [-o --out folder] [-i --index number] [-l --list]" << std::endl;
					std::cerr << std::format("Error {}: Selected model index is invalid", EXIT_INDEX_FAILED_PARSE) << std::endl;
					return EXIT_INDEX_FAILED_PARSE;
				}
				break;
			case 'l':
				listNamesBool = true;
				break;
			default:
				std::cerr << "Usage: gex2ps1modelexporter file [-o --out folder] [-i --index number] [-l --list]" << std::endl;
				std::cerr << std::format("Error {}: Arguments not formatted properly", EXIT_BAD_ARGS) << std::endl;
				return EXIT_BAD_ARGS;
		}
	}


	if (optind < argc)
		inputFile = argv[optind];
	else
	{
		std::cerr << "Usage: gex2ps1modelexporter file [-o --out folder] [-i --index number] [-l --list]" << std::endl;
		std::cerr << std::format("Error {}: Need at least the input file to work", EXIT_INSUFFICIENT_ARGS) << std::endl;
		return EXIT_INSUFFICIENT_ARGS;
	}

	if (!std::filesystem::exists(inputFile))
	{
		// Input file doesn't exist
		std::cerr << std::format("Error {}: Input file does not exist", EXIT_INPUT_NOT_FOUND) << std::endl;
		return EXIT_INPUT_NOT_FOUND;
	}

	std::ifstream reader(inputFile, std::ifstream::binary);
	reader.exceptions(std::ifstream::eofbit);

	if (!reader)
	{
		std::cerr << std::format("Error {}: Failed to read input file", EXIT_INPUT_FAILED_READ) << std::endl;
		return EXIT_INPUT_FAILED_READ;
	}


	if (!std::filesystem::is_directory(outputFolder))
	{
		// Failed to access output folder
		std::cerr << std::format("Error {}: Output directory does not exist", EXIT_OUTPUT_NOT_FOUND) << std::endl;
		return EXIT_OUTPUT_NOT_FOUND;
	}

	outputFolder = outputFolder + directorySeparator() + getFileNameWithoutExtension(inputFile, false);

	bool modelFailedToExport = false;
	bool textureFailedToExport = false;
	bool atLeastOneExportedSuccessfully = false;

	if (readFile(reader, inputFile, outputFolder, selectedModelExport, listNamesBool,
		modelFailedToExport, textureFailedToExport, atLeastOneExportedSuccessfully) == 1)
	{
		// End of stream exception
		std::cerr << std::format("Error {}: End of stream exception", EXIT_END_OF_STREAM) << std::endl;
		return EXIT_END_OF_STREAM;
	}
	
	if (!atLeastOneExportedSuccessfully)
	{
		// No models were successfully exported
		std::cerr << std::format("Error {}: No models were exported successfully", EXIT_ALL_MODELS_FAILED_EXPORT) << std::endl;
		return EXIT_ALL_MODELS_FAILED_EXPORT;
	}
	if (modelFailedToExport)
	{
		std::cerr << std::format("Error {}: At least one model failed to export", EXIT_SOME_MODELS_FAILED_EXPORT) << std::endl;
		return EXIT_SOME_MODELS_FAILED_EXPORT;
	}
	if (textureFailedToExport)
	{
		std::cerr << std::format("Error {}: At least one texture failed to export", EXIT_SOME_TEXTURES_FAILED_EXPORT) << std::endl;
		return EXIT_SOME_TEXTURES_FAILED_EXPORT;
	}
	std::cout << "Exit Code 0: Successful export with no errors" << std::endl;
	return EXIT_SUCCESSFUL_EXPORT;
}



int readFile(std::ifstream& reader, std::string inputFile, std::string outputFolder, int selectedModelExport, bool listNamesBool,
	bool& modelFailedToExport, bool& textureFailedToExport, bool& atLeastOneExportedSuccessfully)
{
	unsigned int modelsAddressesStart;

	try
	{
		initialiseVRM(std::format("{}.vrm", getFileNameWithoutExtension(inputFile, true)));
		unsigned int bitshift;
		reader.read((char*)&bitshift, sizeof(bitshift));
		bitshift = ((bitshift >> 9) << 11) + 0x800;
		reader.seekg(0, reader.end);
		size_t filesize = reader.tellg();
		reader.seekg(bitshift, reader.beg);
	
		std::ofstream tempWriter("Gex2PS1ModelExporterTempfile.drm", std::ifstream::binary);

		while (reader.tellg() < filesize)
		{
			unsigned char data;
			reader.read((char*)&data, sizeof(data));
			tempWriter << data;
		}
		tempWriter.close();

		reader.close();
		reader.open("Gex2PS1ModelExporterTempfile.drm", std::ifstream::binary);

		std::cout << std::format("Reading from {}...", inputFile) << std::endl;

		reader.seekg(0x3C, reader.beg);
		reader.read((char*)&modelsAddressesStart, sizeof(modelsAddressesStart));
		reader.seekg(modelsAddressesStart, reader.beg);

		if (listNamesBool)
			// Break out of sequence entirely, only list names, do not export any models afterwards
			return listNames(reader, modelsAddressesStart);
	}
	catch (std::ifstream::failure &e)
	{
		// End of stream exception
		reader.close();
		std::remove("Gex2PS1ModelExporterTempfile.drm");
		return 1;
	}

	unsigned int objIndex = 0;

	while (selectedModelExport != 0)
	{
		unsigned int specificObjectAddress;
		long int nextPos;
		try
		{
			reader.read((char*)&specificObjectAddress, sizeof(specificObjectAddress));

			if (specificObjectAddress == modelsAddressesStart)
				break;

			objIndex++;

			nextPos = reader.tellg();
		}
		catch (std::ifstream::failure &e)
		{
			// End of stream exception
			reader.close();
			std::remove("Gex2PS1ModelExporterTempfile.drm");
			return 1;
		}

		if (objIndex == 8192)
			break;

		if (objIndex == selectedModelExport || selectedModelExport == -1)
		{
			std::string objName;
			unsigned short int objectCount;
			unsigned int objectStartAddress;

			try
			{
				reader.seekg(specificObjectAddress + 0x24, reader.beg);
				unsigned int objNameAddr;
				reader.read((char*)&objNameAddr, sizeof(objNameAddr));
				reader.seekg(objNameAddr, reader.beg);
				for (int i = 0; i < 8; i++)
				{
					char objNameChar;
					reader.read((char*)&objNameChar, 1);
					objName += objNameChar;
				}

				reader.seekg(specificObjectAddress + 0x8, reader.beg);
				reader.read((char*)&objectCount, sizeof(objectCount));
				reader.seekg(2, reader.cur);
				reader.read((char*)&objectStartAddress, sizeof(objectStartAddress));
			}
			catch (std::ifstream::failure &e)
			{
				reader.seekg(nextPos, reader.beg);
				std::cerr << std::format("Read Error: Error reading metadata of the model at index {}", objIndex) << std::endl;
				continue;
			}

			std::string plural = "";
			if (objectCount > 1)
				plural = "s";
			std::cout << std::format("Found model {} at index {} with {} sub-object{}", objName, objIndex, objectCount, plural) << std::endl;

			for (int i = 0; i < objectCount; i++)
			{
				int objectReturnCode;
				std::string objectNameAndIndex = objName;
				if (objectCount > 1)
					objectNameAndIndex = objName + std::to_string(i + 1);

				try
				{
					reader.seekg(objectStartAddress + (i * 4), reader.beg);
					unsigned int objectModelData;
					reader.read((char*)&objectModelData, sizeof(objectModelData));

					std::cout << std::format("	Reading {}...", objectNameAndIndex) << std::endl;

					reader.seekg(objectModelData, reader.beg);
					objectReturnCode = convertObjToDAE(reader, outputFolder, objectNameAndIndex, inputFile);
				}
				catch (std::ifstream::failure &e)
				{
					objectReturnCode = 2;
				}

				if (!textureFailedToExport && !modelFailedToExport && objectReturnCode == 1)
					textureFailedToExport = true;

				if (objectReturnCode == 2)
				{
					// Model failed to export
					std::cerr << std::format("	Export Error: Model {} failed to export", objectNameAndIndex) << std::endl;
					modelFailedToExport = true;
				}
				else
				{
					atLeastOneExportedSuccessfully = true;
					std::cout << std::format("	Successfully exported {}", objectNameAndIndex) << std::endl;
				}
			}
			if (objIndex == selectedModelExport) { break; }
		}

		reader.seekg(nextPos, reader.beg);
	}
	if (selectedModelExport < 1)
	{
		int levelReturnCode;
		std::cout << std::format("Reading level geometry model {}...", getFileNameWithoutExtension(inputFile, false)) << std::endl;
		try
		{
			reader.seekg(0, reader.beg);
			unsigned int levelData;
			reader.read((char*)&levelData, sizeof(levelData));
			reader.seekg(levelData, reader.beg);

			levelReturnCode = convertLevelToDAE(reader, outputFolder, inputFile);
		}
		catch(std::ifstream::failure &e)
		{
			levelReturnCode = 2;
		}

		if (!textureFailedToExport && !modelFailedToExport && levelReturnCode == 1)
		{
			// At least 1 texture failed to export
			textureFailedToExport = true;
		}

		if (levelReturnCode == 2)
		{
			// Model failed to export
			std::cerr << std::format("	Export Error: Level geometry {} failed to export", getFileNameWithoutExtension(inputFile, false)) << std::endl;
			modelFailedToExport = true;
		}
		else
		{
			atLeastOneExportedSuccessfully = true;
			std::cout << std::format("	Successfully exported level geometry {}", getFileNameWithoutExtension(inputFile, false)) << std::endl;
		}
	}
	reader.close();
	std::remove("Gex2PS1ModelExporterTempfile.drm");

	return 0;
}




int convertObjToDAE(std::ifstream& reader, std::string outputFolder, std::string objectName, std::string inputFile)
{
	unsigned short int vertexCount;
	unsigned int vertexStartAddress;
	unsigned short int polygonCount;
	unsigned int polygonStartAddress;
	unsigned short int boneCount;
	unsigned int boneStartAddress;
	unsigned int textureAnimationsStartAddress;

	reader.read((char*)&vertexCount, sizeof(vertexCount));
	reader.seekg(2, reader.cur);
	reader.read((char*)&vertexStartAddress, sizeof(vertexStartAddress));
	reader.seekg(8, reader.cur);
	reader.read((char*)&polygonCount, sizeof(polygonCount));
	reader.seekg(2, reader.cur);
	reader.read((char*)&polygonStartAddress, sizeof(polygonStartAddress));
	reader.read((char*)&boneCount, sizeof(boneCount));
	reader.seekg(2, reader.cur);
	reader.read((char*)&boneStartAddress, sizeof(boneStartAddress));
	reader.read((char*)&textureAnimationsStartAddress, sizeof(textureAnimationsStartAddress));

	std::vector<Vertex> vertices;

	readVertices(reader, vertexCount, vertexStartAddress, boneCount, boneStartAddress, true, vertices);

	std::vector<PolygonStruct> polygons;
	std::vector<Material> materials;

	std::filesystem::create_directory(outputFolder);

	readPolygons(reader, objectName, outputFolder, polygonCount, polygonStartAddress, textureAnimationsStartAddress, true, polygons, materials, vertices);

	int exportReturn = exportToXML(outputFolder, objectName, polygons, materials);

	return exportReturn;
}

int convertLevelToDAE(std::ifstream& reader, std::string outputFolder, std::string inputFile)
{
	std::string objectName = getFileNameWithoutExtension(inputFile, false);
	unsigned int BSPTreeStartAddress;
	unsigned int vertexCount;
	unsigned int polygonCount;
	unsigned int vertexColourCount;
	unsigned int vertexStartAddress;
	unsigned int polygonStartAddress;
	unsigned int vertexColourStartAddress;
	unsigned int materialStartAddress;

	reader.read((char*)&BSPTreeStartAddress, sizeof(BSPTreeStartAddress));
	reader.seekg(0x14, reader.cur);
	reader.read((char*)&vertexCount, sizeof(vertexCount));
	reader.read((char*)&polygonCount, sizeof(polygonCount));
	reader.read((char*)&vertexColourCount, sizeof(vertexColourCount));
	reader.read((char*)&vertexStartAddress, sizeof(vertexStartAddress));
	reader.read((char*)&polygonStartAddress, sizeof(polygonStartAddress));
	reader.read((char*)&vertexColourStartAddress, sizeof(vertexColourStartAddress));
	reader.read((char*)&materialStartAddress, sizeof(materialStartAddress));

	std::vector<Vertex> vertices;

	readVertices(reader, vertexCount, vertexStartAddress, NULL, NULL, false, vertices);

	// Read vertex colours

	std::vector<PolygonStruct> polygons;
	std::vector<Material> materials;

	std::filesystem::create_directory(outputFolder);

	readPolygons(reader, objectName, outputFolder, polygonCount, polygonStartAddress, materialStartAddress, false, polygons, materials, vertices);

	int exportReturn = exportToXML(outputFolder, objectName, polygons, materials);

	return exportReturn;
}
