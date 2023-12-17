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

#include "tinyxml2.h"
#include "getopt.h"

#include "Gex2PS1ModelExporter.h"
#include "Gex2PS1TextureExporter.h"
#include "Constants.h"
#include <filesystem>
#include <algorithm>
#include <vector>
#include <math.h>

int main(int argc, char* argv[])
{
	std::string inputFile;
	std::string outputFolder = std::filesystem::current_path().string();

	//Selected export -1 = everything
	//Selected export 0 = level geometry
	//Selected export >0 = other object models
	int selectedModelExport = -1;

	static struct option long_options[] =
	{
		{"out", required_argument, NULL, 'o'},
		{"index", required_argument, NULL, 'i'},
		{NULL, 0, NULL, 0}
	};

	int opt;
	while (opt = getopt_long(argc, argv, "o:i:", long_options, NULL) != -1)
	{
		switch (opt)
		{
			case 'o':
				outputFolder = optarg;
				break;
			case 'i':
				if ((selectedModelExport = stringToInt(optarg, -2)) < -1)
				{
					std::cerr << std::format("Usage: {} file [-o --out folder] [-i --index number]", argv[0]) << std::endl;
					std::cerr << std::format("Error {}: Selected model index is invalid", EXIT_INDEX_FAILED_PARSE) << std::endl;
					return EXIT_INDEX_FAILED_PARSE;
				}
				break;
			default:
				std::cerr << std::format("Error {}: Arguments not formatted properly", EXIT_BAD_ARGS) << std::endl;
				return EXIT_BAD_ARGS;
		}
	}

	if (optind < argc)
		inputFile = argv[optind];
	else
	{
		std::cerr << std::format("Error {}: Need at least the input file to work", EXIT_INSUFFICIENT_ARGS) << std::endl;
		return EXIT_INSUFFICIENT_ARGS;
	}
	
	if (!std::filesystem::exists(inputFile))
	{
		//Input file doesn't exist
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
		//Failed to access output folder
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
	}
	catch (std::ifstream::failure &e)
	{
		//End of stream exception
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
			//End of stream exception
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
					//Model failed to export
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
			//At least 1 texture failed to export
			textureFailedToExport = true;
		}

		if (levelReturnCode == 2)
		{
			//Model failed to export
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
		//No models were successfully exported
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

	int exportReturn = XMLExport(outputFolder, objectName, polygons, materials);

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

	//Read vertex colours

	std::vector<PolygonStruct> polygons;
	std::vector<Material> materials;

	std::filesystem::create_directory(outputFolder);

	readPolygons(reader, objectName, outputFolder, polygonCount, polygonStartAddress, materialStartAddress, false, polygons, materials, vertices);

	int exportReturn = XMLExport(outputFolder, objectName, polygons, materials);

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
		//Slightly hacky solution for creating a vector with the needed size
		//This is as bones can have "ancestor IDs"
		//Later on, the program tries access a bone using the index ancestor IDs
		//If there aren't already enough bones in the vector, the program would crash, as it tries to access a bone that is out of bounds of the bones array
		//By doing this beforehand, it makes sure there will not be an out of bounds exception
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
				//Find CLUT value and texture page for frame 1, add all subframes to material
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

				//Find all subframes with same CLUT and texture page, and add the UVs to the material subframes
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
		//For "fake materials", AKA polygons that don't actually have any materials that point to them in the files
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
		//For "fake materials", AKA polygons that don't actually have any materials that point to them in the files
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

	//0x02 = Animated texture flag
	//0x80 = Invisible texture flag
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

	//Translate to bottom left
	for (const unsigned int& polygonID : polygonIDs)
	{
		polygons[polygonID].uv1.u -= leftCoord;
		polygons[polygonID].uv2.u -= leftCoord;
		polygons[polygonID].uv3.u -= leftCoord;
		polygons[polygonID].uv1.v -= southCoord;
		polygons[polygonID].uv2.v -= southCoord;
		polygons[polygonID].uv3.v -= southCoord;
	}

	//Stretch up to top right
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

	//Export textures
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




int XMLExport(std::string outputFolder, std::string objectName, std::vector<PolygonStruct>& polygons, std::vector<Material>& materials)
{
	//Tried using ASSIMP but there was a severe lack of tutorials on how to create a scene from scratch. Pretty much everything online focused solely on importing.
	//So instead, here is my attempt at creating a DAE file using tinyxml2. Enjoy...

	int returnValue = 0;

	struct tm* timePointer;
	std::time_t currentTime = std::time(0);
	char timeString[100];
	timePointer = localtime(&currentTime);
	strftime(timeString, 100, "%FT%T", timePointer);

	tinyxml2::XMLDocument outputDAE;

	tinyxml2::XMLDeclaration* header = outputDAE.NewDeclaration("xml version = \"1.0\" encoding = \"UTF-8\" standalone = \"no\"");
	outputDAE.LinkEndChild(header);

	tinyxml2::XMLElement* rootNode = outputDAE.NewElement("COLLADA");
	rootNode->SetAttribute("xmlns", "http://www.collada.org/2005/11/COLLADASchema");
	rootNode->SetAttribute("version", "1.4.1");
	tinyxml2::XMLElement* asset = outputDAE.NewElement("asset");
	tinyxml2::XMLElement* contributor = outputDAE.NewElement("contributor");
	tinyxml2::XMLElement* author = outputDAE.NewElement("author");
	author->SetText("Crystal Dynamics");
	tinyxml2::XMLElement* authoring_tool = outputDAE.NewElement("authoring_tool");
	authoring_tool->SetText("Gex 2 PS1 Model Exporter");
	contributor->LinkEndChild(author);
	contributor->LinkEndChild(authoring_tool);
	tinyxml2::XMLElement* creationDate = outputDAE.NewElement("created");
	creationDate->SetText(timeString);
	tinyxml2::XMLElement* modifiedDate = outputDAE.NewElement("modified");
	modifiedDate->SetText(timeString);
	tinyxml2::XMLElement* unit = outputDAE.NewElement("unit");
	unit->SetAttribute("name", "meter");
	unit->SetAttribute("meter", "1");
	tinyxml2::XMLElement* up_axis = outputDAE.NewElement("up_axis");
	up_axis->SetText("Z_UP");
	asset->LinkEndChild(contributor);
	asset->LinkEndChild(creationDate);
	asset->LinkEndChild(modifiedDate);
	asset->LinkEndChild(unit);
	asset->LinkEndChild(up_axis);
	rootNode->LinkEndChild(asset);

	tinyxml2::XMLElement* library_images = outputDAE.NewElement("library_images");
	tinyxml2::XMLElement* library_effects = outputDAE.NewElement("library_effects");
	tinyxml2::XMLElement* library_materials = outputDAE.NewElement("library_materials");
	tinyxml2::XMLElement* library_geometries = outputDAE.NewElement("library_geometries");

	tinyxml2::XMLElement* library_visual_scenes = outputDAE.NewElement("library_visual_scenes");
	tinyxml2::XMLElement* visual_scene = outputDAE.NewElement("visual_scene");
	visual_scene->SetAttribute("id", objectName.c_str());
	visual_scene->SetAttribute("name", objectName.c_str());
	tinyxml2::XMLElement* nodeArmature = outputDAE.NewElement("node");
	nodeArmature->SetAttribute("id", std::format("{}node", objectName).c_str());
	nodeArmature->SetAttribute("name", objectName.c_str());
	nodeArmature->SetAttribute("type", "NODE");
	tinyxml2::XMLElement* matrixArmature = outputDAE.NewElement("matrix");
	matrixArmature->SetAttribute("sid", "matrix");
	matrixArmature->SetText("1 0 0 0 0 1 0 0 0 0 1 0 0 0 0 1");
	tinyxml2::XMLElement* nodeModel = outputDAE.NewElement("node");
	nodeModel->SetAttribute("id", std::format("{}node0", objectName).c_str());
	nodeModel->SetAttribute("name", objectName.c_str());
	nodeModel->SetAttribute("type", "NODE");
	tinyxml2::XMLElement* matrixModel = outputDAE.NewElement("matrix");
	matrixModel->SetAttribute("sid", "matrix");
	matrixModel->SetText("1 0 0 0 0 1 0 0 0 0 1 0 0 0 0 1");
	nodeModel->LinkEndChild(matrixModel);

	for (int m = 0; m < materials.size(); m++)
	{
		if (!materials[m].properlyExported)
		{
			returnValue = 1;
		}

		//Textures

		if (materials[m].realMaterial && materials[m].properlyExported)
		{
			tinyxml2::XMLElement* image = outputDAE.NewElement("image");
			image->SetAttribute("id", std::format("{}-{}-diffuse-image", objectName, materials[m].textureID).c_str());
			tinyxml2::XMLElement* init_fromDiffuseImage = outputDAE.NewElement("init_from");
			init_fromDiffuseImage->SetText(std::format("{}-tex{}.png", objectName, materials[m].textureID + 1).c_str());
			image->LinkEndChild(init_fromDiffuseImage);
			library_images->LinkEndChild(image);
		}

		//Effects

		if (materials[m].properlyExported)
		{
			tinyxml2::XMLElement* effect = outputDAE.NewElement("effect");
			effect->SetAttribute("id", std::format("{}-{}-fx", objectName, m).c_str());
			effect->SetAttribute("name", std::format("{}-{}", objectName, m).c_str());
			tinyxml2::XMLElement* profile_COMMON = outputDAE.NewElement("profile_COMMON");

			if (materials[m].realMaterial)
			{
				tinyxml2::XMLElement* newparamSurface = outputDAE.NewElement("newparam");
				newparamSurface->SetAttribute("sid", std::format("{}-{}-diffuse-surface", objectName, materials[m].textureID).c_str());
				tinyxml2::XMLElement* surface = outputDAE.NewElement("surface");
				surface->SetAttribute("type", "2D");
				tinyxml2::XMLElement* init_fromDiffuseSurface = outputDAE.NewElement("init_from");
				init_fromDiffuseSurface->SetText(std::format("{}-{}-diffuse-image", objectName, materials[m].textureID).c_str());
				surface->LinkEndChild(init_fromDiffuseSurface);
				newparamSurface->LinkEndChild(surface);
				profile_COMMON->LinkEndChild(newparamSurface);

				tinyxml2::XMLElement* newparamSampler = outputDAE.NewElement("newparam");
				newparamSampler->SetAttribute("sid", std::format("{}-{}-diffuse-sampler", objectName, materials[m].textureID).c_str());
				tinyxml2::XMLElement* sampler2D = outputDAE.NewElement("sampler2D");
				tinyxml2::XMLElement* samplerSource = outputDAE.NewElement("source");
				samplerSource->SetText(std::format("{}-{}-diffuse-surface", objectName, materials[m].textureID).c_str());
				sampler2D->LinkEndChild(samplerSource);
				newparamSampler->LinkEndChild(sampler2D);
				profile_COMMON->LinkEndChild(newparamSampler);
			}

			tinyxml2::XMLElement* technique = outputDAE.NewElement("technique");
			technique->SetAttribute("sid", "standard");
			tinyxml2::XMLElement* phong = outputDAE.NewElement("phong");
			tinyxml2::XMLElement* diffuse = outputDAE.NewElement("diffuse");

			if (materials[m].realMaterial)
			{
				tinyxml2::XMLElement* texture = outputDAE.NewElement("texture");
				texture->SetAttribute("texture", std::format("{}-{}-diffuse-sampler", objectName, materials[m].textureID).c_str());
				texture->SetAttribute("texcoord", "CHANNEL0");
				diffuse->LinkEndChild(texture);
			}
			else
			{
				std::string coloursString = "";
				coloursString += std::to_string(rgbToLinearRgb(materials[m].redVal) / 1.25f) + " ";
				coloursString += std::to_string(rgbToLinearRgb(materials[m].greenVal) / 1.25f) + " ";
				coloursString += std::to_string(rgbToLinearRgb(materials[m].blueVal) / 1.25f) + " ";
				coloursString += "1";

				tinyxml2::XMLElement* colour = outputDAE.NewElement("color");
				colour->SetAttribute("sid", "diffuse");
				colour->SetText(coloursString.c_str());
				diffuse->LinkEndChild(colour);
			}
			phong->LinkEndChild(diffuse);
			tinyxml2::XMLElement* specular = outputDAE.NewElement("specular");
			tinyxml2::XMLElement* specularColour = outputDAE.NewElement("color");
			specularColour->SetAttribute("sid", "specular");
			specularColour->SetText("0   0   0   0");
			specular->LinkEndChild(specularColour);
			phong->LinkEndChild(specular);
			tinyxml2::XMLElement* transparency = outputDAE.NewElement("transparency");
			tinyxml2::XMLElement* transparencyFloat = outputDAE.NewElement("float");
			transparencyFloat->SetAttribute("sid", "transparency");
			transparencyFloat->SetText("1");
			transparency->LinkEndChild(transparencyFloat);
			phong->LinkEndChild(transparency);
			technique->LinkEndChild(phong);
			profile_COMMON->LinkEndChild(technique);

			effect->LinkEndChild(profile_COMMON);
			library_effects->LinkEndChild(effect);
		}

		//Materials

		tinyxml2::XMLElement* material = outputDAE.NewElement("material");
		material->SetAttribute("id", std::format("{}-mat{}", objectName, m).c_str());
		material->SetAttribute("name", std::format("{}-mat{}", objectName, m).c_str());
		if (materials[m].properlyExported)
		{
			tinyxml2::XMLElement* instance_effectMaterial = outputDAE.NewElement("instance_effect");
			instance_effectMaterial->SetAttribute("url", std::format("#{}-{}-fx", objectName, m).c_str());
			material->LinkEndChild(instance_effectMaterial);
		}
		library_materials->LinkEndChild(material);

		//Geometry Setup

		std::vector<PolygonStruct> meshPolygons;
		std::vector<Vertex> meshVertices;
		for (int p = 0; p < polygons.size(); p++)
		{
			if (polygons[p].materialID == m)
			{
				meshPolygons.push_back(polygons[p]);
				meshVertices.push_back(polygons[p].v1);
				meshVertices.push_back(polygons[p].v2);
				meshVertices.push_back(polygons[p].v3);
			}
		}

		tinyxml2::XMLElement* geometry = outputDAE.NewElement("geometry");
		geometry->SetAttribute("id", std::format("meshId{}", m).c_str());
		geometry->SetAttribute("name", std::format("meshId{}_name", m).c_str());
		tinyxml2::XMLElement* mesh = outputDAE.NewElement("mesh");

		//Positions

		tinyxml2::XMLElement* positionsSource = outputDAE.NewElement("source");
		positionsSource->SetAttribute("id", std::format("meshId{}-positions", m).c_str());
		positionsSource->SetAttribute("name", std::format("meshId{}-positions", m).c_str());
		tinyxml2::XMLElement* positionsFloat_array = outputDAE.NewElement("float_array");
		positionsFloat_array->SetAttribute("id", std::format("meshId{}-positions-array", m).c_str());
		positionsFloat_array->SetAttribute("count", meshPolygons.size() * 9);
		std::string positionsString = " ";
		for (int v = 0; v < meshVertices.size(); v++)
		{
			positionsString += std::format("{} ", divideByAPowerOfTen(meshVertices[v].finalX, 3));
			positionsString += std::format("{} ", divideByAPowerOfTen(meshVertices[v].finalY, 3));
			positionsString += std::format("{} ", divideByAPowerOfTen(meshVertices[v].finalZ, 3));
		}
		positionsFloat_array->SetText(positionsString.c_str());
		tinyxml2::XMLElement* positionsTechnique_common = outputDAE.NewElement("technique_common");
		tinyxml2::XMLElement* positionsAccessor = outputDAE.NewElement("accessor");
		positionsAccessor->SetAttribute("count", meshPolygons.size() * 3);
		positionsAccessor->SetAttribute("offset", 0);
		positionsAccessor->SetAttribute("source", std::format("#meshId{}-positions-array", m).c_str());
		positionsAccessor->SetAttribute("stride", 3);
		tinyxml2::XMLElement* paramX = outputDAE.NewElement("param");
		paramX->SetAttribute("name", "X");
		paramX->SetAttribute("type", "float");
		tinyxml2::XMLElement* paramY = outputDAE.NewElement("param");
		paramY->SetAttribute("name", "Y");
		paramY->SetAttribute("type", "float");
		tinyxml2::XMLElement* paramZ = outputDAE.NewElement("param");
		paramZ->SetAttribute("name", "Z");
		paramZ->SetAttribute("type", "float");

		positionsAccessor->LinkEndChild(paramX);
		positionsAccessor->LinkEndChild(paramY);
		positionsAccessor->LinkEndChild(paramZ);
		positionsTechnique_common->LinkEndChild(positionsAccessor);
		positionsSource->LinkEndChild(positionsFloat_array);
		positionsSource->LinkEndChild(positionsTechnique_common);
		mesh->LinkEndChild(positionsSource);

		//Textures

		tinyxml2::XMLElement* texturesSource = outputDAE.NewElement("source");
		texturesSource->SetAttribute("id", std::format("meshId{}-tex", m).c_str());
		texturesSource->SetAttribute("name", std::format("meshId{}-tex", m).c_str());
		tinyxml2::XMLElement* texturesFloat_array = outputDAE.NewElement("float_array");
		texturesFloat_array->SetAttribute("id", std::format("meshId{}-tex-array", m).c_str());
		texturesFloat_array->SetAttribute("count", meshPolygons.size() * 6);
		std::string texturesString = " ";
		for (int p = 0; p < meshPolygons.size(); p++)
		{
			texturesString += std::format("{} ", meshPolygons[p].uv1.u);
			texturesString += std::format("{} ", meshPolygons[p].uv1.v);
			texturesString += std::format("{} ", meshPolygons[p].uv2.u);
			texturesString += std::format("{} ", meshPolygons[p].uv2.v);
			texturesString += std::format("{} ", meshPolygons[p].uv3.u);
			texturesString += std::format("{} ", meshPolygons[p].uv3.v);
		}
		texturesFloat_array->SetText(texturesString.c_str());
		tinyxml2::XMLElement* texturesTechnique_common = outputDAE.NewElement("technique_common");
		tinyxml2::XMLElement* texturesAccessor = outputDAE.NewElement("accessor");
		texturesAccessor->SetAttribute("count", meshPolygons.size() * 3);
		texturesAccessor->SetAttribute("offset", 0);
		texturesAccessor->SetAttribute("source", std::format("#meshId{}-tex-array", m).c_str());
		texturesAccessor->SetAttribute("stride", 2);
		tinyxml2::XMLElement* paramS = outputDAE.NewElement("param");
		paramS->SetAttribute("name", "S");
		paramS->SetAttribute("type", "float");
		tinyxml2::XMLElement* paramT = outputDAE.NewElement("param");
		paramT->SetAttribute("name", "T");
		paramT->SetAttribute("type", "float");

		texturesAccessor->LinkEndChild(paramS);
		texturesAccessor->LinkEndChild(paramT);
		texturesTechnique_common->LinkEndChild(texturesAccessor);
		texturesSource->LinkEndChild(texturesFloat_array);
		texturesSource->LinkEndChild(texturesTechnique_common);
		mesh->LinkEndChild(texturesSource);

		//Colours

		tinyxml2::XMLElement* coloursSource = outputDAE.NewElement("source");
		coloursSource->SetAttribute("id", std::format("meshId{}-color", m).c_str());
		coloursSource->SetAttribute("name", std::format("meshId{}-color", m).c_str());
		tinyxml2::XMLElement* coloursFloat_array = outputDAE.NewElement("float_array");
		coloursFloat_array->SetAttribute("id", std::format("meshId{}-colors-array", m).c_str());
		coloursFloat_array->SetAttribute("count", meshPolygons.size() * 9);
		std::string coloursString = " ";
		for (int c = 0; c < meshPolygons.size() * 3; c++)
		{
			//Temporary thing as I'm not sure how colours work in the models yet
			if (!materials[m].realMaterial)
			{
				coloursString += std::to_string(rgbToLinearRgb(materials[m].redVal) / 1.25f) + " ";
				coloursString += std::to_string(rgbToLinearRgb(materials[m].greenVal) / 1.25f) + " ";
				coloursString += std::to_string(rgbToLinearRgb(materials[m].blueVal) / 1.25f) + " ";
			}
			else
				coloursString += "0 0 0 ";
		}
		coloursFloat_array->SetText(coloursString.c_str());
		tinyxml2::XMLElement* coloursTechnique_common = outputDAE.NewElement("technique_common");
		tinyxml2::XMLElement* coloursAccessor = outputDAE.NewElement("accessor");
		coloursAccessor->SetAttribute("count", meshPolygons.size() * 3);
		coloursAccessor->SetAttribute("offset", 0);
		coloursAccessor->SetAttribute("source", std::format("#meshId{}-color-array", m).c_str());
		coloursAccessor->SetAttribute("stride", 3);
		tinyxml2::XMLElement* paramR = outputDAE.NewElement("param");
		paramR->SetAttribute("name", "R");
		paramR->SetAttribute("type", "float");
		tinyxml2::XMLElement* paramG = outputDAE.NewElement("param");
		paramG->SetAttribute("name", "G");
		paramG->SetAttribute("type", "float");
		tinyxml2::XMLElement* paramB = outputDAE.NewElement("param");
		paramB->SetAttribute("name", "B");
		paramB->SetAttribute("type", "float");

		coloursAccessor->LinkEndChild(paramR);
		coloursAccessor->LinkEndChild(paramG);
		coloursAccessor->LinkEndChild(paramB);
		coloursTechnique_common->LinkEndChild(coloursAccessor);
		coloursSource->LinkEndChild(coloursFloat_array);
		coloursSource->LinkEndChild(coloursTechnique_common);
		mesh->LinkEndChild(coloursSource);

		//Vertices

		tinyxml2::XMLElement* xVertices = outputDAE.NewElement("vertices");
		xVertices->SetAttribute("id", std::format("meshId{}-vertices", m).c_str());
		tinyxml2::XMLElement* input = outputDAE.NewElement("input");
		input->SetAttribute("semantic", "POSITION");
		input->SetAttribute("source", std::format("#meshId{}-positions", m).c_str());
		xVertices->LinkEndChild(input);
		mesh->LinkEndChild(xVertices);

		//Polygons

		tinyxml2::XMLElement* polylist = outputDAE.NewElement("polylist");
		polylist->SetAttribute("count", meshPolygons.size());
		polylist->SetAttribute("material", "defaultMaterial");
		tinyxml2::XMLElement* inputVertex = outputDAE.NewElement("input");
		inputVertex->SetAttribute("offset", 0);
		inputVertex->SetAttribute("semantic", "VERTEX");
		inputVertex->SetAttribute("source", std::format("#meshId{}-vertices", m).c_str());
		tinyxml2::XMLElement* inputTexCoord = outputDAE.NewElement("input");
		inputTexCoord->SetAttribute("offset", 0);
		inputTexCoord->SetAttribute("semantic", "TEXCOORD");
		inputTexCoord->SetAttribute("source", std::format("#meshId{}-tex", m).c_str());
		inputTexCoord->SetAttribute("set", 0);
		tinyxml2::XMLElement* inputColour = outputDAE.NewElement("input");
		inputColour->SetAttribute("offset", 0);
		inputColour->SetAttribute("semantic", "COLOR");
		inputColour->SetAttribute("source", std::format("#meshId{}-color", m).c_str());
		inputColour->SetAttribute("set", 0);
		tinyxml2::XMLElement* vCount = outputDAE.NewElement("vcount");
		tinyxml2::XMLElement* pPolylist = outputDAE.NewElement("p");
		std::string vCountString = "";
		std::string polyString = "";
		for (int p = 0; p < meshPolygons.size(); p++)
		{
			vCountString += "3 ";
			polyString += std::format("{} {} {} ", (p * 3), (p * 3) + 1, (p * 3) + 2);
		}
		vCount->SetText(vCountString.c_str());
		pPolylist->SetText(polyString.c_str());

		polylist->LinkEndChild(inputVertex);
		polylist->LinkEndChild(inputTexCoord);
		polylist->LinkEndChild(inputColour);
		polylist->LinkEndChild(vCount);
		polylist->LinkEndChild(pPolylist);
		mesh->LinkEndChild(polylist);

		//Linking geometry

		geometry->LinkEndChild(mesh);
		library_geometries->LinkEndChild(geometry);

		//Visual Scenes

		tinyxml2::XMLElement* instance_geometry = outputDAE.NewElement("instance_geometry");
		instance_geometry->SetAttribute("url", std::format("#meshId{}", m).c_str());
		tinyxml2::XMLElement* bind_material = outputDAE.NewElement("bind_material");
		tinyxml2::XMLElement* visualSceneTechnique_common = outputDAE.NewElement("technique_common");
		tinyxml2::XMLElement* instance_material = outputDAE.NewElement("instance_material");
		instance_material->SetAttribute("symbol", "defaultMaterial");
		instance_material->SetAttribute("target", std::format("#{}-mat{}", objectName, m).c_str());
		tinyxml2::XMLElement* bind_vertex_input = outputDAE.NewElement("bind_vertex_input");
		bind_vertex_input->SetAttribute("semantic", "CHANNEL0");
		bind_vertex_input->SetAttribute("input_semantic", "TEXCOORD");
		bind_vertex_input->SetAttribute("input_set", 0);

		instance_material->LinkEndChild(bind_vertex_input);
		visualSceneTechnique_common->LinkEndChild(instance_material);
		bind_material->LinkEndChild(visualSceneTechnique_common);
		instance_geometry->LinkEndChild(bind_material);
		nodeModel->LinkEndChild(instance_geometry);
	}

	rootNode->LinkEndChild(library_images);
	rootNode->LinkEndChild(library_effects);
	rootNode->LinkEndChild(library_materials);
	rootNode->LinkEndChild(library_geometries);

	nodeArmature->LinkEndChild(matrixArmature);
	nodeArmature->LinkEndChild(nodeModel);
	visual_scene->LinkEndChild(nodeArmature);
	library_visual_scenes->LinkEndChild(visual_scene);
	rootNode->LinkEndChild(library_visual_scenes);


	tinyxml2::XMLElement* scene = outputDAE.NewElement("scene");
	tinyxml2::XMLElement* instance_visual_scene = outputDAE.NewElement("instance_visual_scene");
	instance_visual_scene->SetAttribute("url", std::format("#{}", objectName).c_str());
	scene->LinkEndChild(instance_visual_scene);
	rootNode->LinkEndChild(scene);

	outputDAE.LinkEndChild(rootNode);

	if (std::filesystem::exists(outputFolder))
	{
		outputDAE.SaveFile(std::format("{}{}{}.dae", outputFolder, directorySeparator(), objectName).c_str());
		return returnValue;
	}

	//Could not export
	return 2;
}


float rgbToLinearRgb(unsigned char colour)
{
	if (colour <= 10)
		return (float)(colour / 255.0f / 12.92f);
	else
		return (float)(std::pow(((colour / 255.0f) + 0.055f) / 1.055f, 2.4));
}