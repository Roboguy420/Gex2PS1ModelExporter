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

#pragma once

#include "ModelStructs.h"
#include "TextureStructs.h"
#include "SharedFunctions.h"
#include <iostream>
#include <stdio.h>
#include <fstream>
#include <string>
#include <format>

int convertObjToDAE(std::ifstream& reader, std::string outputFolder, std::string objectName, std::string inputFile);
int convertLevelToDAE(std::ifstream& reader, std::string outputFolder, std::string inputFile);
void readVertices(std::ifstream& reader, unsigned short int vertexCount, unsigned int vertexStartAddress, unsigned short int boneCount, unsigned int boneStartAddress, bool isObject, std::vector<Vertex>& vertices);
Vertex readVertex(std::ifstream& reader, unsigned int v);
void readArmature(std::ifstream& reader, unsigned short int boneCount, unsigned int boneStartAddress, std::vector<Bone>& bones);
void applyArmature(std::ifstream& reader, unsigned short int vertexCount, unsigned int vertexStartAddress, unsigned short int boneCount, unsigned int boneStartAddress, std::vector<Vertex>& vertices, std::vector<Bone>& bones);
void readPolygons(std::ifstream& reader, std::string objectName, std::string outputFolder, unsigned short int polygonCount, unsigned int polygonStartAddress, unsigned int textureAnimationsStartAddress, bool isObject, std::vector<PolygonStruct>& polygons, std::vector<Material>& materials, std::vector<Vertex>& vertices);
PolygonStruct readPolygon(std::ifstream& reader, unsigned int p, int materialStartAddress, bool isObject, std::vector<Material>& materials, std::vector<Vertex>& vertices, std::vector<ObjectAnimationSubframe>& subframes);
void readObjectPolygon(std::ifstream& reader, PolygonStruct& thisPolygon, Material& thisMaterial, bool& realMaterial, unsigned int& materialAddress);
void readLevelPolygon(std::ifstream& reader, PolygonStruct& thisPolygon, Material& thisMaterial, bool& realMaterial, unsigned int& materialAddress);
Material readMaterial(std::ifstream& reader);
std::vector<ObjectAnimationSubframe> readObjectAnimationSubFrames(std::ifstream& reader, unsigned int textureAnimationsStartAddress);
ObjectAnimationSubframe readObjectAnimationSubFrame(std::ifstream& reader, unsigned int baseMaterialAddress);
std::vector<LevelAnimationSubframe> readLevelAnimationSubFrames(std::ifstream& reader, unsigned int textureAnimationsStartAddress);
LevelAnimationSubframe* readLevelAnimationSubFrame(std::ifstream& reader, unsigned int baseMaterialAddress);
bool UVPointCorrectionAndExport(unsigned int materialID, bool isObject, std::string objectName, std::string outputFolder, Material thisMaterial, std::vector<PolygonStruct>& polygons, bool exportLevelAnimations, std::vector<LevelAnimationSubframe>& levelSubframes);
bool objectSubframePointCorrectionAndExport(unsigned int materialID, unsigned int textureID, std::string objectName, std::string outputFolder, ObjectAnimationSubframe subframe);

int stringToInt(std::string inputString, int failValue)
{
	for (int i = 0; i < inputString.length(); i++)
	{
		if (i == 0 && inputString[i] == '-')
			continue;
		if (!isdigit(inputString[i]))
			return failValue;
	}
	return atoi(inputString.c_str());
}

std::string getFileNameWithoutExtension(std::string fileName, bool includePath)
{
	size_t parentDirEnd = fileName.find_last_of(directorySeparator());
	std::string filePath;
	if (parentDirEnd != std::string::npos)
	{
		filePath = fileName.substr(0, parentDirEnd + 1);
		fileName = fileName.substr(parentDirEnd + 1);
	}
	size_t extensionStart = fileName.find_last_of('.');
	if (extensionStart == std::string::npos)
	{
		return fileName; //Simply returns the filename if there is no extension
	}
	if (includePath)
	{
		return filePath + fileName.substr(0, extensionStart);
	}
	return fileName.substr(0, extensionStart);
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