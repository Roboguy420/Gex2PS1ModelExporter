﻿/*  Gex2PS1ModelExporter: Command line program for exporting Gex 2 PS1 models 
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

#include "tinyxml2.h"
#include "getopt.h"

#include "ModelExporter.h"
#include "TextureExporter.h"
#include "ModelNamesLister.h"
#include "XMLExport.h"
#include "Constants.h"

#include <filesystem>
#include <algorithm>
#include <vector>
#include <math.h>

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
					std::cerr << std::format("Usage: {} file [-o --out folder] [-i --index number] [-l --list]", argv[0]) << std::endl;
					std::cerr << std::format("Error {}: Selected model index is invalid", EXIT_INDEX_FAILED_PARSE) << std::endl;
					return EXIT_INDEX_FAILED_PARSE;
				}
				break;
			case 'l':
				listNamesBool = true;
				break;
			default:
				std::cerr << std::format("Usage: {} file [-o --out folder] [-i --index number] [-l --list]", argv[0]) << std::endl;
				std::cerr << std::format("Error {}: Arguments not formatted properly", EXIT_BAD_ARGS) << std::endl;
				return EXIT_BAD_ARGS;
		}
	}


	if (optind < argc)
		inputFile = argv[optind];
	else
	{
		std::cerr << std::format("Usage: {} file [-o --out folder] [-i --index number]", argv[0]) << std::endl;
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
		std::cerr << std::format("Error {}: End of stream exception", EXIT_END_OF_STREAM) << std::endl;
		return EXIT_END_OF_STREAM;
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
			std::cerr << std::format("Error {}: End of stream exception", EXIT_END_OF_STREAM) << std::endl;
			return EXIT_END_OF_STREAM;
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




void readVertices(std::ifstream& reader, unsigned short int vertexCount, unsigned int vertexStartAddress, unsigned short int boneCount, unsigned int boneStartAddress, bool isObject, std::vector<Vertex>& vertices)
{
	if (vertexStartAddress == 0 || vertexCount == 0) { return; }

	reader.seekg(vertexStartAddress, reader.beg);

	for (unsigned int v = 0; v < vertexCount; v++)
	{
		unsigned int uPolygonPosition = reader.tellg();
		vertices.push_back(readVertex(reader, v));
		reader.seekg(uPolygonPosition + 0xC, reader.beg);
	}

	if (isObject)
	{
		std::vector<Bone> bones;

		readArmature(reader, boneCount, boneStartAddress, bones);

		applyArmature(reader, vertexCount, vertexStartAddress, boneCount, boneStartAddress, vertices, bones);
	}
}

Vertex readVertex(std::ifstream& reader, unsigned int v)
{
	Vertex thisVertex;

	thisVertex.positionID = v;

	short int x;
	short int y;
	short int z;

	reader.read((char*)&x, 2);
	reader.read((char*)&y, 2);
	reader.read((char*)&z, 2);

	thisVertex.rawX = x;
	thisVertex.rawY = y;
	thisVertex.rawZ = z;
	thisVertex.finalX = x;
	thisVertex.finalY = y;
	thisVertex.finalZ = z;

	reader.read((char*)&thisVertex.normalID, 2);

	return thisVertex;
}




void readArmature(std::ifstream &reader, unsigned short int boneCount, unsigned int boneStartAddress, std::vector<Bone>& bones)
{
	if (boneStartAddress == 0 || boneCount == 0) { return; }

	reader.seekg(boneStartAddress, reader.beg);

	for (unsigned short int b = 0; b < boneCount; b++)
	{
		// Slightly hacky solution for creating a vector with the needed size
		// This is as bones can have "ancestor IDs"
		// Later on, the program tries access a bone using the index ancestor IDs
		// If there aren't already enough bones in the vector, the program would crash, as it tries to access a bone that is out of bounds of the bones array
		// By doing this beforehand, it makes sure there will not be an out of bounds exception
		Bone nullBone;
		bones.push_back(nullBone);
	}

	for (unsigned short int b = 0; b < boneCount; b++)
	{
		reader.seekg(8, reader.cur);

		reader.read((char*)&bones[b].vFirst, sizeof(bones[b].vFirst));
		reader.read((char*)&bones[b].vLast, sizeof(bones[b].vLast));
		reader.read((char*)&bones[b].localX, 2);
		reader.read((char*)&bones[b].localY, 2);
		reader.read((char*)&bones[b].localZ, 2);
		reader.read((char*)&bones[b].parentID, sizeof(bones[b].parentID));

		bones[b].worldX = 0.0f;
		bones[b].worldY = 0.0f;
		bones[b].worldZ = 0.0f;

		if ((bones[b].vFirst != 0xFFFF) && (bones[b].vLast != 0xFFFF))
		{
			for (unsigned short int ancestorID = b; ancestorID != 0xFFFF;)
			{
				bones[b].worldX += bones[ancestorID].localX;
				bones[b].worldY += bones[ancestorID].localY;
				bones[b].worldZ += bones[ancestorID].localZ;
				if (bones[ancestorID].parentID == ancestorID) { break; }
				ancestorID = bones[ancestorID].parentID;
			}
		}

		reader.seekg(4, reader.cur);
	}
}

void applyArmature(std::ifstream& reader, unsigned short int vertexCount, unsigned int vertexStartAddress, unsigned short int boneCount, unsigned int boneStartAddress, std::vector<Vertex>& vertices, std::vector<Bone>& bones)
{
	if (vertexStartAddress == 0 || vertexCount == 0 || boneStartAddress == 0 || boneCount == 0) { return; }

	for (unsigned short int b = 0; b < boneCount; b++)
	{
		if (bones[b].vFirst != 0xFFFF && bones[b].vLast != 0xFFFF)
		{
			for (unsigned short int v = bones[b].vFirst; v <= bones[b].vLast; v++)
			{
				vertices[v].finalX += bones[b].worldX;
				vertices[v].finalY += bones[b].worldY;
				vertices[v].finalZ += bones[b].worldZ;
				vertices[v].boneID = b;
			}
		}
	}
}




void readPolygons(std::ifstream& reader, std::string objectName, std::string outputFolder, unsigned short int polygonCount, unsigned int polygonStartAddress, unsigned int textureAnimationsStartAddress, bool isObject, std::vector<PolygonStruct>& polygons, std::vector<Material>& materials, std::vector<Vertex>& vertices)
{
	if (polygonStartAddress == 0 || polygonCount == 0) { return; }

	std::vector<ObjectAnimationSubframe> objectSubframes;
	std::vector<LevelAnimationSubframe> levelSubframes;

	if (textureAnimationsStartAddress != 0)
	{
		if (isObject)
			objectSubframes = readObjectAnimationSubFrames(reader, textureAnimationsStartAddress);
		else
			levelSubframes = readLevelAnimationSubFrames(reader, textureAnimationsStartAddress);
	}

	reader.seekg(polygonStartAddress, reader.beg);

	for (unsigned short int p = 0; p < polygonCount; p++)
	{
		unsigned int uPolygonPosition = reader.tellg();
		polygons.push_back(readPolygon(reader, p, textureAnimationsStartAddress, isObject, materials, vertices, objectSubframes));
		if (isObject) reader.seekg(uPolygonPosition + 0xC, reader.beg);
		else reader.seekg(uPolygonPosition + 0x14, reader.beg);
	}

	for (unsigned int m = 0; m < materials.size(); m++)
	{
		if (materials[m].realMaterial)
		{
			materials[m].properlyExported = UVPointCorrectionAndExport(m, isObject, objectName, outputFolder, materials[m], polygons, !isObject, levelSubframes);

			for (unsigned int i = 0; i < materials[m].objectSubframes.size(); i++)
			{
				objectSubframePointCorrectionAndExport(m, materials[m].textureID, objectName, outputFolder, materials[m].objectSubframes[i]);
			}
		}
	}
}

std::vector<ObjectAnimationSubframe> readObjectAnimationSubFrames(std::ifstream& reader, unsigned int textureAnimationsStartAddress)
{
	std::vector<ObjectAnimationSubframe> objectSubframes;

	reader.seekg(textureAnimationsStartAddress, reader.beg);
	unsigned int textureAnimationsCount;
	reader.read((char*)&textureAnimationsCount, sizeof(textureAnimationsCount));
	for (int i = 0; i < textureAnimationsCount; i++)
	{
		unsigned int uTextureAnimationsPosition = reader.tellg();
		unsigned int materialAddress;
		unsigned int subframesCount;
		reader.read((char*)&materialAddress, sizeof(materialAddress));
		reader.read((char*)&subframesCount, sizeof(subframesCount));
		reader.seekg(materialAddress + 0x10, reader.beg);
		for (int m = 0; m < subframesCount; m++)
		{
			unsigned int uSubframePosition = reader.tellg();
			objectSubframes.push_back(readObjectAnimationSubFrame(reader, materialAddress));
			objectSubframes[objectSubframes.size() - 1].subframeID = m;
			reader.seekg(uSubframePosition + 0x10, reader.beg);
		}
		reader.seekg(uTextureAnimationsPosition + 0xC, reader.beg);
	}

	return objectSubframes;
}

ObjectAnimationSubframe readObjectAnimationSubFrame(std::ifstream &reader, unsigned int baseMaterialAddress)
{
	ObjectAnimationSubframe subframe;

	unsigned char u[3];
	unsigned char v[3];
	reader.read((char*)&u[0], 1);
	reader.read((char*)&v[0], 1);
	reader.read((char*)&subframe.clutValue, sizeof(subframe.clutValue));
	reader.read((char*)&u[1], 1);
	reader.read((char*)&v[1], 1);
	reader.read((char*)&subframe.texturePage, sizeof(subframe.texturePage));
	reader.read((char*)&u[2], 1);
	reader.read((char*)&v[2], 1);

	subframe.UVs.push_back({ u[0] / 255.0f, (255 - v[0]) / 255.0f });
	subframe.UVs.push_back({ u[1] / 255.0f, (255 - v[1]) / 255.0f });
	subframe.UVs.push_back({ u[2] / 255.0f, (255 - v[2]) / 255.0f });

	subframe.baseMaterialAddress = baseMaterialAddress;

	return subframe;
}

std::vector<LevelAnimationSubframe> readLevelAnimationSubFrames(std::ifstream& reader, unsigned int textureAnimationsStartAddress)
{
	std::vector<LevelAnimationSubframe> levelSubframes;

	reader.seekg(textureAnimationsStartAddress, reader.beg);
	unsigned int textureAnimationsCount;
	reader.read((char*)&textureAnimationsCount, sizeof(textureAnimationsCount));
	for (unsigned int i = 0; i < textureAnimationsCount; i++)
	{
		unsigned int uTextureAnimationsPosition = reader.tellg();
		unsigned int materialAddress;
		reader.read((char*)&materialAddress, sizeof(materialAddress));
		reader.seekg(materialAddress, reader.beg);
		LevelAnimationSubframe* subframesPointer = readLevelAnimationSubFrame(reader, materialAddress);
		levelSubframes.push_back(subframesPointer[0]);
		levelSubframes.push_back(subframesPointer[1]);
		reader.seekg(uTextureAnimationsPosition + 4, reader.beg);
	}

	return levelSubframes;
}

LevelAnimationSubframe* readLevelAnimationSubFrame(std::ifstream &reader, unsigned int baseMaterialAddress)
{
	LevelAnimationSubframe* subframes = new LevelAnimationSubframe[2];

	reader.read((char*)&(subframes[0].xCoordinateDestination), sizeof(subframes[0].xCoordinateDestination));
	reader.read((char*)&(subframes[0].yCoordinateDestination), sizeof(subframes[0].yCoordinateDestination));
	reader.read((char*)&(subframes[0].xSize), sizeof(subframes[0].xSize));
	reader.read((char*)&(subframes[0].ySize), sizeof(subframes[0].ySize));

	reader.read((char*)&(subframes[1].xCoordinateDestination), sizeof(subframes[1].xCoordinateDestination));
	reader.read((char*)&(subframes[1].yCoordinateDestination), sizeof(subframes[1].yCoordinateDestination));
	reader.read((char*)&(subframes[1].xSize), sizeof(subframes[1].xSize));
	reader.read((char*)&(subframes[1].ySize), sizeof(subframes[1].ySize));

	subframes[0].xCoordinateDestination -= 0x200;
	subframes[1].xCoordinateDestination -= 0x200;

	reader.seekg(8, reader.cur);

	unsigned int numberOfFrames;
	reader.read((char*)&numberOfFrames, sizeof(numberOfFrames));
	reader.seekg(4, reader.cur);

	for (unsigned int frame = 0; frame < numberOfFrames; frame++)
	{
		unsigned short int xCoordinateSource1, yCoordinateSource1, xCoordinateSource2, yCoordinateSource2;

		reader.read((char*)&xCoordinateSource1, sizeof(xCoordinateSource1));
		reader.read((char*)&yCoordinateSource1, sizeof(yCoordinateSource1));
		reader.read((char*)&xCoordinateSource2, sizeof(xCoordinateSource2));
		reader.read((char*)&yCoordinateSource2, sizeof(yCoordinateSource2));

		xCoordinateSource1 -= 0x200;
		xCoordinateSource2 -= 0x200;

		subframes[0].xCoordinateSources.push_back(xCoordinateSource1);
		subframes[0].yCoordinateSources.push_back(yCoordinateSource1);
		subframes[1].xCoordinateSources.push_back(xCoordinateSource2);
		subframes[1].yCoordinateSources.push_back(yCoordinateSource2);
	}

	subframes[0].subframeExportsThis = false;
	subframes[1].subframeExportsThis = false;

	return subframes;
}

PolygonStruct readPolygon(std::ifstream& reader, unsigned int p, int materialStartAddress, bool isObject, std::vector<Material>& materials, std::vector<Vertex>& vertices, std::vector<ObjectAnimationSubframe>& subframes)
{
	PolygonStruct thisPolygon;

	unsigned short int v1Index;
	unsigned short int v2Index;
	unsigned short int v3Index;

	reader.read((char*)&v1Index, 2);
	reader.read((char*)&v2Index, 2);
	reader.read((char*)&v3Index, 2);

	thisPolygon.v1 = vertices[v1Index];
	thisPolygon.v2 = vertices[v2Index];
	thisPolygon.v3 = vertices[v3Index];

	Material thisMaterial;
	bool realMaterial = true;

	unsigned int materialAddress;

	if (isObject)
		readObjectPolygon(reader, thisPolygon, thisMaterial, realMaterial, materialAddress);
	else
		readLevelPolygon(reader, thisPolygon, thisMaterial, realMaterial, materialAddress);


	if (realMaterial)
	{
		bool newMaterial = true;
		for (int m = 0; m < materials.size(); m++)
		{
			bool newClutValue = true;
			bool newTexturePage = true;
			if (thisMaterial.clutValue == materials[m].clutValue)
			{
				newClutValue = false;
			}
			if (thisMaterial.texturePage == materials[m].texturePage)
			{
				newTexturePage = false;
			}

			if (!newClutValue && !newTexturePage)
			{
				newMaterial = false;
				thisPolygon.materialID = m;
				break;
			}
		}

		if (newMaterial)
		{
			unsigned int textureCount = 0;
			for (unsigned int i = 0; i < materials.size(); i++)
			{
				if (materials[i].realMaterial)
				{
					textureCount++;
				}
			}
			thisMaterial.textureID = textureCount;

			if (isObject)
			{
				// Find CLUT value and texture page for frame 1, add all subframes to material
				unsigned int subframeIncrement = 0;
				int subframeClutValue = -1;
				int subframeTexturePage = -1;
				for (unsigned int i = 0; i < subframes.size(); i++)
				{
					if (subframes[i].baseMaterialAddress == materialAddress)
					{
						subframes[i].subframeID = subframeIncrement;
						thisMaterial.objectSubframes.push_back(subframes[i]);
						subframeIncrement++;
						if (subframeClutValue == -1)
						{
							subframeClutValue = subframes[i].clutValue;
							subframeTexturePage = subframes[i].texturePage;
						}
					}
				}

				// Find all subframes with same CLUT and texture page, and add the UVs to the material subframes
				unsigned int a = subframes.size();
				unsigned int b = 0;
				for (unsigned int i = 0; i < subframes.size(); i++)
				{
					if (subframes[i].subframeID == 0 && subframes[i].clutValue == subframeClutValue && subframes[i].texturePage == subframeTexturePage && subframes[i].baseMaterialAddress != materialAddress)
					{
						for (unsigned int j = 0; j < subframeIncrement * 3; j++)
						{
							thisMaterial.objectSubframes[j / 3].UVs.push_back(subframes[i + (j / 3)].UVs[j % 3]);
						}
					}
				}
			}

			materials.push_back(thisMaterial);
			thisPolygon.materialID = materials.size() - 1;
		}
	}
	else
	{
		// For "fake materials", AKA polygons that don't actually have any materials that point to them in the files
		bool newMaterial = true;
		for (int m = 0; m < materials.size(); m++)
		{
			if (!materials[m].realMaterial && thisMaterial.redVal == materials[m].redVal
			&& thisMaterial.greenVal == materials[m].greenVal && thisMaterial.blueVal == materials[m].blueVal)
			{
				newMaterial = false;
				thisPolygon.materialID = m;
				break;
			}
		}
		if (newMaterial)
		{
			thisMaterial.realMaterial = false;
			thisMaterial.visible = true;
			thisMaterial.properlyExported = true;
			materials.push_back(thisMaterial);
			thisPolygon.materialID = materials.size() - 1;
		}
		thisPolygon.uv1.u = 0.0f;
		thisPolygon.uv2.u = 0.0f;
		thisPolygon.uv3.u = 0.0f;
		thisPolygon.uv1.v = 255.0f;
		thisPolygon.uv2.v = 255.0f;
		thisPolygon.uv3.v = 255.0f;
	}

	return thisPolygon;
}

void readObjectPolygon(std::ifstream& reader, PolygonStruct& thisPolygon, Material& thisMaterial, bool& realMaterial, unsigned int& materialAddress)
{
	reader.seekg(1, reader.cur);

	unsigned char polygonFlags;
	reader.read((char*)&polygonFlags, sizeof(polygonFlags));
	thisMaterial.visible = true;

	if ((polygonFlags & 0x02) == 0x02)
	{
		realMaterial = true;
		reader.read((char*)&materialAddress, sizeof(materialAddress));

		reader.seekg(materialAddress, reader.beg);

		unsigned char u[3];
		unsigned char v[3];
		reader.read((char*)&u[0], 1);
		reader.read((char*)&v[0], 1);
		reader.seekg(2, reader.cur);
		reader.read((char*)&u[1], 1);
		reader.read((char*)&v[1], 1);
		reader.seekg(2, reader.cur);
		reader.read((char*)&u[2], 1);
		reader.read((char*)&v[2], 1);

		thisPolygon.uv1.u = u[0] / 255.0f;
		thisPolygon.uv1.v = (255 - v[0]) / 255.0f;
		thisPolygon.uv2.u = u[1] / 255.0f;
		thisPolygon.uv2.v = (255 - v[1]) / 255.0f;
		thisPolygon.uv3.u = u[2] / 255.0f;
		thisPolygon.uv3.v = (255 - v[2]) / 255.0f;

		reader.seekg(materialAddress, reader.beg);
		thisMaterial = readMaterial(reader);
	}
	else
	{
		// For "fake materials", AKA polygons that don't actually have any materials that point to them in the files
		realMaterial = false;

		reader.read((char*)&thisMaterial.redVal, 1);
		reader.read((char*)&thisMaterial.greenVal, 1);
		reader.read((char*)&thisMaterial.blueVal, 1);
	}
}

void readLevelPolygon(std::ifstream& reader, PolygonStruct& thisPolygon, Material& thisMaterial, bool& realMaterial, unsigned int& materialAddress)
{
	unsigned char polygonFlags;
	reader.seekg(0x1, reader.cur);
	reader.read((char*)&polygonFlags, sizeof(polygonFlags));
	reader.seekg(0x8, reader.cur);

	reader.read((char*)&materialAddress, sizeof(materialAddress));

	// 0x02 = Animated texture flag
	// 0x80 = Invisible texture flag
	if (materialAddress != 0xFFFF && (polygonFlags & 0x80) != 0x80)
	{
		reader.seekg(materialAddress, reader.beg);

		unsigned char u[3];
		unsigned char v[3];
		reader.read((char*)&u[0], 1);
		reader.read((char*)&v[0], 1);
		reader.seekg(2, reader.cur);
		reader.read((char*)&u[1], 1);
		reader.read((char*)&v[1], 1);
		reader.seekg(2, reader.cur);
		reader.read((char*)&u[2], 1);
		reader.read((char*)&v[2], 1);

		thisPolygon.uv1.u = u[0] / 255.0f;
		thisPolygon.uv1.v = (255 - v[0]) / 255.0f;
		thisPolygon.uv2.u = u[1] / 255.0f;
		thisPolygon.uv2.v = (255 - v[1]) / 255.0f;
		thisPolygon.uv3.u = u[2] / 255.0f;
		thisPolygon.uv3.v = (255 - v[2]) / 255.0f;

		reader.seekg(materialAddress, reader.beg);
		thisMaterial = readMaterial(reader);
	}
	else
		realMaterial = false;
}




Material readMaterial(std::ifstream& reader)
{
	Material thisMaterial;
	thisMaterial.realMaterial = true;

	reader.seekg(2, reader.cur);
	reader.read((char*)&thisMaterial.clutValue, sizeof(thisMaterial.clutValue));
	reader.seekg(2, reader.cur);
	reader.read((char*)&thisMaterial.texturePage, sizeof(thisMaterial.texturePage));
	reader.seekg(2, reader.cur);

	return thisMaterial;
}

bool UVPointCorrectionAndExport(unsigned int materialID, bool isObject, std::string objectName, std::string outputFolder, Material thisMaterial, std::vector<PolygonStruct>& polygons, bool exportLevelAnimations, std::vector<LevelAnimationSubframe>& levelSubframes)
{
	std::vector<UV> materialUVs;
	std::vector<unsigned int> polygonIDs;

	for (unsigned int p = 0; p < polygons.size(); p++)
	{
		if (polygons[p].materialID == materialID)
		{
			polygonIDs.push_back(p);
			materialUVs.push_back(polygons[p].uv1);
			materialUVs.push_back(polygons[p].uv2);
			materialUVs.push_back(polygons[p].uv3);
		}
	}

	std::sort(materialUVs.begin(), materialUVs.end(), sortUCoord);
	float leftCoord = materialUVs[0].u;
	float rightCoord = materialUVs[materialUVs.size() - 1].u;
	std::sort(materialUVs.begin(), materialUVs.end(), sortVCoord);
	float southCoord = materialUVs[0].v;
	float northCoord = materialUVs[materialUVs.size() - 1].v;

	// Translate to bottom left
	for (const unsigned int& polygonID : polygonIDs)
	{
		polygons[polygonID].uv1.u -= leftCoord;
		polygons[polygonID].uv2.u -= leftCoord;
		polygons[polygonID].uv3.u -= leftCoord;
		polygons[polygonID].uv1.v -= southCoord;
		polygons[polygonID].uv2.v -= southCoord;
		polygons[polygonID].uv3.v -= southCoord;
	}

	// Stretch up to top right
	float stretchInU = 1.0f / (rightCoord - leftCoord);
	float stretchInV = 1.0f / (northCoord - southCoord);
	if (rightCoord - leftCoord == 0)
		stretchInU = 0.0f;
	if (northCoord - southCoord == 0)
		stretchInV = 0.0f;

	for (const unsigned int& polygonID : polygonIDs)
	{
		polygons[polygonID].uv1.u *= stretchInU;
		polygons[polygonID].uv2.u *= stretchInU;
		polygons[polygonID].uv3.u *= stretchInU;
		polygons[polygonID].uv1.v *= stretchInV;
		polygons[polygonID].uv2.v *= stretchInV;
		polygons[polygonID].uv3.v *= stretchInV;
	}

	// Export textures
	unsigned int leftCoordInt = floor(leftCoord * 255.0f + 0.5f);
	unsigned int rightCoordInt = floor(rightCoord * 255.0f + 0.5f);
	unsigned int southCoordInt = 255 - floor(southCoord * 255.0f + 0.5f);
	unsigned int northCoordInt = 255 - floor(northCoord * 255.0f + 0.5f);

	int texPageReturnValue;
	texPageReturnValue = goToTexPageAndApplyCLUT(thisMaterial.texturePage, thisMaterial.clutValue, leftCoordInt, rightCoordInt, southCoordInt, northCoordInt, objectName, outputFolder, (thisMaterial.textureID + 1), materialID, 0, levelSubframes);
	if (exportLevelAnimations)
	{
		std::vector<LevelAnimationSubframe> empty;
		for (unsigned int i = 0; i < levelSubframes.size(); i+= 2)
		{
			if (levelSubframes[i].subframeExportsThis)
			{
				for (unsigned int j = 0; j < levelSubframes[i].xCoordinateSources.size(); j++)
				{
					copyRectangleInVRM(levelSubframes[i].xCoordinateDestination, levelSubframes[i].yCoordinateDestination, levelSubframes[i].xSize,
						levelSubframes[i].ySize, levelSubframes[i].xCoordinateSources[j], levelSubframes[i].yCoordinateSources[j], true);
					copyRectangleInVRM(levelSubframes[i + 1].xCoordinateDestination, levelSubframes[i + 1].yCoordinateDestination, levelSubframes[i + 1].xSize,
						levelSubframes[i + 1].ySize, levelSubframes[i + 1].xCoordinateSources[j], levelSubframes[i + 1].yCoordinateSources[j], true);

					if (goToTexPageAndApplyCLUT(thisMaterial.texturePage, thisMaterial.clutValue, leftCoordInt, rightCoordInt, southCoordInt, northCoordInt, objectName, outputFolder, (thisMaterial.textureID + 1), materialID, j + 1, empty) != 0)
						std::cerr << std::format("	Export Error: Level subframe texture {}-tex{}-{}.png failed to export", objectName, (thisMaterial.textureID + 1), (j + 1)) << std::endl;
				}
			}
			levelSubframes[i].subframeExportsThis = false;
		}
		resetModifiedVRAM();
	}
	if (texPageReturnValue != 0)
	{
		std::cerr << std::format("	Export Error: Texture {}-tex{}.png failed to export", objectName, (thisMaterial.textureID + 1)) << std::endl;
		return false;
	}
	return true;
}

bool objectSubframePointCorrectionAndExport(unsigned int materialID, unsigned int textureID, std::string objectName, std::string outputFolder, ObjectAnimationSubframe subframe)
{
	std::sort(subframe.UVs.begin(), subframe.UVs.end(), sortUCoord);
	float leftCoord = subframe.UVs[0].u;
	float rightCoord = subframe.UVs[subframe.UVs.size() - 1].u;
	std::sort(subframe.UVs.begin(), subframe.UVs.end(), sortVCoord);
	float southCoord = subframe.UVs[0].v;
	float northCoord = subframe.UVs[subframe.UVs.size() - 1].v;

	unsigned int leftCoordInt = floor(leftCoord * 255.0f + 0.5f);
	unsigned int rightCoordInt = floor(rightCoord * 255.0f + 0.5f);
	unsigned int southCoordInt = 255 - floor(southCoord * 255.0f + 0.5f);
	unsigned int northCoordInt = 255 - floor(northCoord * 255.0f + 0.5f);

	std::vector<LevelAnimationSubframe> empty;
	int texPageReturnValue = goToTexPageAndApplyCLUT(subframe.texturePage, subframe.clutValue, leftCoordInt, rightCoordInt, southCoordInt, northCoordInt, objectName, outputFolder, (textureID + 1), materialID, (subframe.subframeID + 1), empty);
	if (texPageReturnValue != 0)
	{
		std::cerr << std::format("	Export Error: Object subframe texture {}-tex{}-{}.png failed to export", objectName, (textureID + 1), (subframe.subframeID + 1)) << std::endl;
		return false;
	}
	return true;
}
