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
#include "Globals.h"
#include "Constants.h"

#include <format>
#include <filesystem>
#include <vector>
#include <iostream>
#include <math.h>
#include <getopt.h>

std::string tempFile = std::format("{}{}Gex2PS1ModelExporterTempfile.drm", tempDirectory(), directorySeparator());

int main(int argc, char* argv[])
{
	g_outputFolder = std::filesystem::current_path().string();

	// Selected export -1 = everything
	// Selected export 0 = level geometry
	// Selected export >0 = other object models
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
				g_outputFolder = optarg;
				break;
			case 'i':
				if ((g_selectedModelExport = stringToInt(optarg, -2)) < -1)
				{
					std::cerr << "Usage: gex2ps1modelexporter file [-o --out folder] [-i --index number] [-l --list]" << std::endl;
					std::cerr << std::format("Error {}: Selected model index is invalid", EXIT_INDEX_FAILED_PARSE) << std::endl;
					return EXIT_INDEX_FAILED_PARSE;
				}
				break;
			case 'l':
				g_listNames = true;
				break;
			default:
				std::cerr << "Usage: gex2ps1modelexporter file [-o --out folder] [-i --index number] [-l --list]" << std::endl;
				std::cerr << std::format("Error {}: Arguments not formatted properly", EXIT_BAD_ARGS) << std::endl;
				return EXIT_BAD_ARGS;
		}
	}


	if (optind < argc)
		g_inputFile = argv[optind];
	else
	{
		std::cerr << "Usage: gex2ps1modelexporter file [-o --out folder] [-i --index number] [-l --list]" << std::endl;
		std::cerr << std::format("Error {}: Need at least the input file to work", EXIT_INSUFFICIENT_ARGS) << std::endl;
		return EXIT_INSUFFICIENT_ARGS;
	}

	if (!std::filesystem::exists(g_inputFile))
	{
		// Input file doesn't exist
		std::cerr << std::format("Error {}: Input file does not exist", EXIT_INPUT_NOT_FOUND) << std::endl;
		return EXIT_INPUT_NOT_FOUND;
	}

	g_reader = std::ifstream(g_inputFile, std::ifstream::binary);
	g_reader.exceptions(std::ifstream::eofbit);

	if (!g_reader.is_open())
	{
		std::cerr << std::format("Error {}: Failed to read input file", EXIT_INPUT_FAILED_READ) << std::endl;
		return EXIT_INPUT_FAILED_READ;
	}


	if (!std::filesystem::is_directory(g_outputFolder))
	{
		// Failed to access output folder
		std::cerr << std::format("Error {}: Output directory does not exist", EXIT_OUTPUT_NOT_FOUND) << std::endl;
		return EXIT_OUTPUT_NOT_FOUND;
	}

	g_outputFolder = g_outputFolder + directorySeparator() + getFileNameWithoutExtension(g_inputFile, false);

	switch (readFile())
	{
		case 1:
			// End of stream exception
			std::cerr << std::format("Error {}: End of stream exception", EXIT_END_OF_STREAM) << std::endl;
			return EXIT_END_OF_STREAM;
		case 2:
			// Failed to write temp file to temp dir
			std::cerr << std::format("Error {}: Failed to write tempfile", EXIT_TEMPFILE_FAILED_WRITE) << std::endl;
			return EXIT_TEMPFILE_FAILED_WRITE;
		case 3:
			// Failed to read temp file
			std::cerr << std::format("Error {}: Failed to read tempfile", EXIT_TEMPFILE_FAILED_READ) << std::endl;
			return EXIT_TEMPFILE_FAILED_READ;
	}
	

	if (g_listNames)
		return EXIT_SUCCESSFUL_EXPORT;
	
	if (!g_atLeastOneExportedSuccessfully)
	{
		// No models were successfully exported
		std::cerr << std::format("Error {}: No models were exported successfully", EXIT_ALL_MODELS_FAILED_EXPORT) << std::endl;
		return EXIT_ALL_MODELS_FAILED_EXPORT;
	}
	if (g_modelFailedToExport)
	{
		std::cerr << std::format("Error {}: At least one model failed to export", EXIT_SOME_MODELS_FAILED_EXPORT) << std::endl;
		return EXIT_SOME_MODELS_FAILED_EXPORT;
	}
	if (g_textureFailedToExport)
	{
		std::cerr << std::format("Error {}: At least one texture failed to export", EXIT_SOME_TEXTURES_FAILED_EXPORT) << std::endl;
		return EXIT_SOME_TEXTURES_FAILED_EXPORT;
	}
	std::cout << "Exit Code 0: Successful export with no errors" << std::endl;
	return EXIT_SUCCESSFUL_EXPORT;
}



int readFile()
{
	unsigned int modelsAddressesStart;

	try
	{
		initialiseVRM(std::format("{}.vrm", getFileNameWithoutExtension(g_inputFile, true)));
		unsigned int bitshift;
		g_reader.read((char*)&bitshift, sizeof(bitshift));
		bitshift = ((bitshift >> 9) << 11) + 0x800;
		g_reader.seekg(0, g_reader.end);
		size_t filesize = g_reader.tellg();
		g_reader.seekg(bitshift, g_reader.beg);
	
		std::ofstream tempWriter(tempFile.c_str(), std::ifstream::binary);

		if (!tempWriter.is_open())
			return 2;

		while (g_reader.tellg() < filesize)
		{
			unsigned char data;
			g_reader.read((char*)&data, sizeof(data));
			tempWriter << data;
		}
		tempWriter.close();

		g_reader.close();
		g_reader.open(tempFile.c_str(), std::ifstream::binary);

		if (!g_reader.is_open())
			return 3;

		std::cout << std::format("Reading from {}...", g_inputFile) << std::endl;

		g_reader.seekg(0x3C, g_reader.beg);
		g_reader.read((char*)&modelsAddressesStart, sizeof(modelsAddressesStart));
		g_reader.seekg(modelsAddressesStart, g_reader.beg);

		if (g_listNames)
		{
			// Break out of sequence entirely, only list names, do not export any models afterwards
			int listNamesReturn = listNames(modelsAddressesStart);
			g_reader.close();
			std::remove(tempFile.c_str());
			return listNamesReturn;
		}
	}
	catch (std::ifstream::failure &e)
	{
		// End of stream exception
		g_reader.close();
		std::remove(tempFile.c_str());
		return 1;
	}

	unsigned int objIndex = 0;

	while (g_selectedModelExport != 0)
	{
		unsigned int specificObjectAddress;
		long int nextPos;
		try
		{
			g_reader.read((char*)&specificObjectAddress, sizeof(specificObjectAddress));

			if (specificObjectAddress == modelsAddressesStart)
				break;

			objIndex++;

			nextPos = g_reader.tellg();
		}
		catch (std::ifstream::failure &e)
		{
			// End of stream exception
			g_reader.close();
			std::remove(tempFile.c_str());
			return 1;
		}

		if (objIndex == 8192)
			break;

		if (objIndex == g_selectedModelExport || g_selectedModelExport == -1)
		{
			std::string objName;
			unsigned short int objectCount;
			unsigned int objectStartAddress;

			try
			{
				g_reader.seekg(specificObjectAddress + 0x24, g_reader.beg);
				unsigned int objNameAddr;
				g_reader.read((char*)&objNameAddr, sizeof(objNameAddr));
				g_reader.seekg(objNameAddr, g_reader.beg);
				for (int i = 0; i < 8; i++)
				{
					char objNameChar;
					g_reader.read((char*)&objNameChar, 1);
					objName += objNameChar;
				}

				g_reader.seekg(specificObjectAddress + 0x8, g_reader.beg);
				g_reader.read((char*)&objectCount, sizeof(objectCount));
				g_reader.seekg(2, g_reader.cur);
				g_reader.read((char*)&objectStartAddress, sizeof(objectStartAddress));
			}
			catch (std::ifstream::failure &e)
			{
				g_reader.seekg(nextPos, g_reader.beg);
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
					g_reader.seekg(objectStartAddress + (i * 4), g_reader.beg);
					unsigned int objectModelData;
					g_reader.read((char*)&objectModelData, sizeof(objectModelData));

					std::cout << std::format("	Reading {}...", objectNameAndIndex) << std::endl;

					g_reader.seekg(objectModelData, g_reader.beg);
					objectReturnCode = convertObjToDAE(objectNameAndIndex);
				}
				catch (std::ifstream::failure &e)
				{
					objectReturnCode = 2;
				}

				if (!g_textureFailedToExport && !g_modelFailedToExport && objectReturnCode == 1)
					g_textureFailedToExport = true;

				if (objectReturnCode == 2)
				{
					// Model failed to export
					std::cerr << std::format("	Export Error: Model {} failed to export", objectNameAndIndex) << std::endl;
					g_modelFailedToExport = true;
				}
				else
				{
					g_atLeastOneExportedSuccessfully = true;
					std::cout << std::format("	Successfully exported {}", objectNameAndIndex) << std::endl;
				}
			}
			if (objIndex == g_selectedModelExport) { break; }
		}

		g_reader.seekg(nextPos, g_reader.beg);
	}
	if (g_selectedModelExport < 1)
	{
		int levelReturnCode;
		std::cout << std::format("Reading level geometry model {}...", getFileNameWithoutExtension(g_inputFile, false)) << std::endl;
		try
		{
			g_reader.seekg(0, g_reader.beg);
			unsigned int levelData;
			g_reader.read((char*)&levelData, sizeof(levelData));
			g_reader.seekg(levelData, g_reader.beg);

			levelReturnCode = convertLevelToDAE();
		}
		catch(std::ifstream::failure &e)
		{
			levelReturnCode = 2;
		}

		if (!g_textureFailedToExport && !g_modelFailedToExport && levelReturnCode == 1)
		{
			// At least 1 texture failed to export
			g_textureFailedToExport = true;
		}

		if (levelReturnCode == 2)
		{
			// Model failed to export
			std::cerr << std::format("	Export Error: Level geometry {} failed to export", getFileNameWithoutExtension(g_inputFile, false)) << std::endl;
			g_modelFailedToExport = true;
		}
		else
		{
			g_atLeastOneExportedSuccessfully = true;
			std::cout << std::format("	Successfully exported level geometry {}", getFileNameWithoutExtension(g_inputFile, false)) << std::endl;
		}
	}
	g_reader.close();
	std::remove(tempFile.c_str());

	return 0;
}




int convertObjToDAE(std::string objectName)
{
	unsigned short int vertexCount;
	unsigned int vertexStartAddress;
	unsigned short int polygonCount;
	unsigned int polygonStartAddress;
	unsigned short int boneCount;
	unsigned int boneStartAddress;
	unsigned int textureAnimationsStartAddress;

	g_reader.read((char*)&vertexCount, sizeof(vertexCount));
	g_reader.seekg(2, g_reader.cur);
	g_reader.read((char*)&vertexStartAddress, sizeof(vertexStartAddress));
	g_reader.seekg(8, g_reader.cur);
	g_reader.read((char*)&polygonCount, sizeof(polygonCount));
	g_reader.seekg(2, g_reader.cur);
	g_reader.read((char*)&polygonStartAddress, sizeof(polygonStartAddress));
	g_reader.read((char*)&boneCount, sizeof(boneCount));
	g_reader.seekg(2, g_reader.cur);
	g_reader.read((char*)&boneStartAddress, sizeof(boneStartAddress));
	g_reader.read((char*)&textureAnimationsStartAddress, sizeof(textureAnimationsStartAddress));

	std::vector<Vertex> vertices;

	readVertices(vertexCount, vertexStartAddress, boneCount, boneStartAddress, true, vertices);

	std::vector<PolygonStruct> polygons;
	std::vector<Material> materials;

	std::filesystem::create_directory(g_outputFolder);

	readPolygons(objectName, polygonCount, polygonStartAddress, textureAnimationsStartAddress, true, polygons, materials, vertices);

	int exportReturn = exportToXML(objectName, polygons, materials);

	return exportReturn;
}

int convertLevelToDAE()
{
	std::string objectName = getFileNameWithoutExtension(g_inputFile, false);
	unsigned int BSPTreeStartAddress;
	unsigned int vertexCount;
	unsigned int polygonCount;
	unsigned int vertexColourCount;
	unsigned int vertexStartAddress;
	unsigned int polygonStartAddress;
	unsigned int vertexColourStartAddress;
	unsigned int materialStartAddress;

	g_reader.read((char*)&BSPTreeStartAddress, sizeof(BSPTreeStartAddress));
	g_reader.seekg(0x14, g_reader.cur);
	g_reader.read((char*)&vertexCount, sizeof(vertexCount));
	g_reader.read((char*)&polygonCount, sizeof(polygonCount));
	g_reader.read((char*)&vertexColourCount, sizeof(vertexColourCount));
	g_reader.read((char*)&vertexStartAddress, sizeof(vertexStartAddress));
	g_reader.read((char*)&polygonStartAddress, sizeof(polygonStartAddress));
	g_reader.read((char*)&vertexColourStartAddress, sizeof(vertexColourStartAddress));
	g_reader.read((char*)&materialStartAddress, sizeof(materialStartAddress));

	std::vector<Vertex> vertices;

	readVertices(vertexCount, vertexStartAddress, NULL, NULL, false, vertices);

	// Read vertex colours

	std::vector<PolygonStruct> polygons;
	std::vector<Material> materials;

	std::filesystem::create_directory(g_outputFolder);

	readPolygons(objectName, polygonCount, polygonStartAddress, materialStartAddress, false, polygons, materials, vertices);

	int exportReturn = exportToXML(objectName, polygons, materials);

	return exportReturn;
}
