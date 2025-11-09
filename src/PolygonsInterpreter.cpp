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

#include "Globals.h"
#include "PolygonsInterpreter.h"
#include "TextureExporter.h"

#include <cmath>
#include <fstream>
#include <iostream>
#include <string>
#include <format>
#include <algorithm>

void readPolygons(std::string objectName, unsigned short int polygonCount,
    unsigned int polygonStartAddress, unsigned int textureAnimationsStartAddress, bool isObject, std::vector<PolygonStruct>& polygons,
    std::vector<Material>& materials, std::vector<Vertex>& vertices)
{
	if (polygonStartAddress == 0 || polygonCount == 0) { return; }

	std::vector<ObjectAnimationSubframe> objectSubframes;
	std::vector<LevelAnimationSubframe> levelSubframes;

	if (textureAnimationsStartAddress != 0)
	{
		if (isObject)
			objectSubframes = readObjectAnimationSubFrames(textureAnimationsStartAddress);
		else
			levelSubframes = readLevelAnimationSubFrames(textureAnimationsStartAddress);
	}

	g_reader.seekg(polygonStartAddress, g_reader.beg);

	for (unsigned short int p = 0; p < polygonCount; p++)
	{
		unsigned int uPolygonPosition = g_reader.tellg();
		polygons.push_back(readPolygon(p, textureAnimationsStartAddress, isObject, materials, vertices, objectSubframes));
		if (isObject) g_reader.seekg(uPolygonPosition + 0xC, g_reader.beg);
		else g_reader.seekg(uPolygonPosition + 0x14, g_reader.beg);
	}

	for (unsigned int m = 0; m < materials.size(); m++)
	{
		if (materials[m].realMaterial)
		{
			materials[m].properlyExported = UVPointCorrectionAndExport(m, isObject, objectName, materials[m], polygons, !isObject, levelSubframes);

			for (unsigned int i = 0; i < materials[m].objectSubframes.size(); i++)
			{
				objectSubframePointCorrectionAndExport(m, materials[m].textureID, objectName, materials[m].objectSubframes[i]);
			}
		}
	}
}

std::vector<ObjectAnimationSubframe> readObjectAnimationSubFrames(unsigned int textureAnimationsStartAddress)
{
	std::vector<ObjectAnimationSubframe> objectSubframes;

	g_reader.seekg(textureAnimationsStartAddress, g_reader.beg);
	unsigned int textureAnimationsCount;
	g_reader.read((char*)&textureAnimationsCount, sizeof(textureAnimationsCount));
	for (int i = 0; i < textureAnimationsCount; i++)
	{
		unsigned int uTextureAnimationsPosition = g_reader.tellg();
		unsigned int materialAddress;
		unsigned int subframesCount;
		g_reader.read((char*)&materialAddress, sizeof(materialAddress));
		g_reader.read((char*)&subframesCount, sizeof(subframesCount));
		g_reader.seekg(materialAddress + 0x10, g_reader.beg);
		for (int m = 0; m < subframesCount; m++)
		{
			unsigned int uSubframePosition = g_reader.tellg();
			objectSubframes.push_back(readObjectAnimationSubFrame(materialAddress));
			objectSubframes[objectSubframes.size() - 1].subframeID = m;
			g_reader.seekg(uSubframePosition + 0x10, g_reader.beg);
		}
		g_reader.seekg(uTextureAnimationsPosition + 0xC, g_reader.beg);
	}

	return objectSubframes;
}

ObjectAnimationSubframe readObjectAnimationSubFrame(unsigned int baseMaterialAddress)
{
	ObjectAnimationSubframe subframe;

	unsigned char u[3];
	unsigned char v[3];
	g_reader.read((char*)&u[0], 1);
	g_reader.read((char*)&v[0], 1);
	g_reader.read((char*)&subframe.clutValue, sizeof(subframe.clutValue));
	g_reader.read((char*)&u[1], 1);
	g_reader.read((char*)&v[1], 1);
	g_reader.read((char*)&subframe.texturePage, sizeof(subframe.texturePage));
	g_reader.read((char*)&u[2], 1);
	g_reader.read((char*)&v[2], 1);

	subframe.UVs.push_back({ u[0] / 255.0f, (255 - v[0]) / 255.0f });
	subframe.UVs.push_back({ u[1] / 255.0f, (255 - v[1]) / 255.0f });
	subframe.UVs.push_back({ u[2] / 255.0f, (255 - v[2]) / 255.0f });

	subframe.baseMaterialAddress = baseMaterialAddress;

	return subframe;
}

std::vector<LevelAnimationSubframe> readLevelAnimationSubFrames(unsigned int textureAnimationsStartAddress)
{
	std::vector<LevelAnimationSubframe> levelSubframes;

	g_reader.seekg(textureAnimationsStartAddress, g_reader.beg);
	unsigned int textureAnimationsCount;
	g_reader.read((char*)&textureAnimationsCount, sizeof(textureAnimationsCount));
	for (unsigned int i = 0; i < textureAnimationsCount; i++)
	{
		unsigned int uTextureAnimationsPosition = g_reader.tellg();
		unsigned int materialAddress;
		g_reader.read((char*)&materialAddress, sizeof(materialAddress));
		g_reader.seekg(materialAddress, g_reader.beg);
		LevelAnimationSubframe* subframesPointer = readLevelAnimationSubFrame(materialAddress);
		levelSubframes.push_back(subframesPointer[0]);
		levelSubframes.push_back(subframesPointer[1]);
		g_reader.seekg(uTextureAnimationsPosition + 4, g_reader.beg);
	}

	return levelSubframes;
}

LevelAnimationSubframe* readLevelAnimationSubFrame(unsigned int baseMaterialAddress)
{
	LevelAnimationSubframe* subframes = new LevelAnimationSubframe[2];

	g_reader.read((char*)&(subframes[0].xCoordinateDestination), sizeof(subframes[0].xCoordinateDestination));
	g_reader.read((char*)&(subframes[0].yCoordinateDestination), sizeof(subframes[0].yCoordinateDestination));
	g_reader.read((char*)&(subframes[0].xSize), sizeof(subframes[0].xSize));
	g_reader.read((char*)&(subframes[0].ySize), sizeof(subframes[0].ySize));

	g_reader.read((char*)&(subframes[1].xCoordinateDestination), sizeof(subframes[1].xCoordinateDestination));
	g_reader.read((char*)&(subframes[1].yCoordinateDestination), sizeof(subframes[1].yCoordinateDestination));
	g_reader.read((char*)&(subframes[1].xSize), sizeof(subframes[1].xSize));
	g_reader.read((char*)&(subframes[1].ySize), sizeof(subframes[1].ySize));

	subframes[0].xCoordinateDestination -= 0x200;
	subframes[1].xCoordinateDestination -= 0x200;

	g_reader.seekg(8, g_reader.cur);

	unsigned int numberOfFrames;
	g_reader.read((char*)&numberOfFrames, sizeof(numberOfFrames));
	g_reader.seekg(4, g_reader.cur);

	for (unsigned int frame = 0; frame < numberOfFrames; frame++)
	{
		unsigned short int xCoordinateSource1, yCoordinateSource1, xCoordinateSource2, yCoordinateSource2;

		g_reader.read((char*)&xCoordinateSource1, sizeof(xCoordinateSource1));
		g_reader.read((char*)&yCoordinateSource1, sizeof(yCoordinateSource1));
		g_reader.read((char*)&xCoordinateSource2, sizeof(xCoordinateSource2));
		g_reader.read((char*)&yCoordinateSource2, sizeof(yCoordinateSource2));

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

PolygonStruct readPolygon(unsigned int p, int materialStartAddress, bool isObject,
    std::vector<Material>& materials, std::vector<Vertex>& vertices, std::vector<ObjectAnimationSubframe>& subframes)
{
	PolygonStruct thisPolygon;

	unsigned short int v1Index;
	unsigned short int v2Index;
	unsigned short int v3Index;

	g_reader.read((char*)&v1Index, 2);
	g_reader.read((char*)&v2Index, 2);
	g_reader.read((char*)&v3Index, 2);

	thisPolygon.v1 = vertices[v1Index];
	thisPolygon.v2 = vertices[v2Index];
	thisPolygon.v3 = vertices[v3Index];

	Material thisMaterial;
	bool realMaterial = true;

	unsigned int materialAddress;

	if (isObject)
		readObjectPolygon(thisPolygon, thisMaterial, realMaterial, materialAddress);
	else
		readLevelPolygon(thisPolygon, thisMaterial, realMaterial, materialAddress);


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

void readObjectPolygon(PolygonStruct& thisPolygon, Material& thisMaterial, bool& realMaterial, unsigned int& materialAddress)
{
	g_reader.seekg(1, g_reader.cur);

	unsigned char polygonFlags;
	g_reader.read((char*)&polygonFlags, sizeof(polygonFlags));
	thisMaterial.visible = true;

	if ((polygonFlags & 0x02) == 0x02)
	{
		realMaterial = true;
		g_reader.read((char*)&materialAddress, sizeof(materialAddress));

		g_reader.seekg(materialAddress, g_reader.beg);

		unsigned char u[3];
		unsigned char v[3];
		g_reader.read((char*)&u[0], 1);
		g_reader.read((char*)&v[0], 1);
		g_reader.seekg(2, g_reader.cur);
		g_reader.read((char*)&u[1], 1);
		g_reader.read((char*)&v[1], 1);
		g_reader.seekg(2, g_reader.cur);
		g_reader.read((char*)&u[2], 1);
		g_reader.read((char*)&v[2], 1);

		thisPolygon.uv1.u = u[0] / 255.0f;
		thisPolygon.uv1.v = (255 - v[0]) / 255.0f;
		thisPolygon.uv2.u = u[1] / 255.0f;
		thisPolygon.uv2.v = (255 - v[1]) / 255.0f;
		thisPolygon.uv3.u = u[2] / 255.0f;
		thisPolygon.uv3.v = (255 - v[2]) / 255.0f;

		if (g_materialsMap.contains(materialAddress))
		{
			thisMaterial = g_materialsMap[materialAddress];
			return;
		}

		g_reader.seekg(materialAddress, g_reader.beg);
		thisMaterial = readMaterial();
		g_materialsMap[materialAddress] = thisMaterial;
	}
	else
	{
		// For "fake materials", AKA polygons that don't actually have any materials that point to them in the files
		realMaterial = false;

		g_reader.read((char*)&thisMaterial.redVal, 1);
		g_reader.read((char*)&thisMaterial.greenVal, 1);
		g_reader.read((char*)&thisMaterial.blueVal, 1);
	}
}

void readLevelPolygon(PolygonStruct& thisPolygon, Material& thisMaterial, bool& realMaterial, unsigned int& materialAddress)
{
	unsigned char polygonFlags;
	g_reader.seekg(0x1, g_reader.cur);
	g_reader.read((char*)&polygonFlags, sizeof(polygonFlags));
	g_reader.seekg(0x8, g_reader.cur);

	g_reader.read((char*)&materialAddress, sizeof(materialAddress));



	// 0x02 = Animated texture flag
	// 0x80 = Invisible texture flag
	if (materialAddress != 0xFFFF && (polygonFlags & 0x80) != 0x80)
	{
		g_reader.seekg(materialAddress, g_reader.beg);

		unsigned char u[3];
		unsigned char v[3];
		g_reader.read((char*)&u[0], 1);
		g_reader.read((char*)&v[0], 1);
		g_reader.seekg(2, g_reader.cur);
		g_reader.read((char*)&u[1], 1);
		g_reader.read((char*)&v[1], 1);
		g_reader.seekg(2, g_reader.cur);
		g_reader.read((char*)&u[2], 1);
		g_reader.read((char*)&v[2], 1);

		thisPolygon.uv1.u = u[0] / 255.0f;
		thisPolygon.uv1.v = (255 - v[0]) / 255.0f;
		thisPolygon.uv2.u = u[1] / 255.0f;
		thisPolygon.uv2.v = (255 - v[1]) / 255.0f;
		thisPolygon.uv3.u = u[2] / 255.0f;
		thisPolygon.uv3.v = (255 - v[2]) / 255.0f;

	  if (g_materialsMap.contains(materialAddress))
	  {
		  thisMaterial = g_materialsMap[materialAddress];
		  return;
	  }

		g_reader.seekg(materialAddress, g_reader.beg);
		thisMaterial = readMaterial();
		g_materialsMap[materialAddress] = thisMaterial;
	}
	else
		realMaterial = false;
}




Material readMaterial()
{
	Material thisMaterial;
	thisMaterial.realMaterial = true;

	g_reader.seekg(2, g_reader.cur);
	g_reader.read((char*)&thisMaterial.clutValue, sizeof(thisMaterial.clutValue));
	g_reader.seekg(2, g_reader.cur);
	g_reader.read((char*)&thisMaterial.texturePage, sizeof(thisMaterial.texturePage));
	g_reader.seekg(2, g_reader.cur);

	return thisMaterial;
}

bool UVPointCorrectionAndExport(unsigned int materialID, bool isObject, std::string objectName, Material thisMaterial,
    std::vector<PolygonStruct>& polygons, bool exportLevelAnimations, std::vector<LevelAnimationSubframe>& levelSubframes)
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
	texPageReturnValue = goToTexPageAndApplyCLUT(thisMaterial.texturePage, thisMaterial.clutValue, leftCoordInt, rightCoordInt,
        southCoordInt, northCoordInt, objectName, (thisMaterial.textureID + 1), materialID, 0, levelSubframes);

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

					if (goToTexPageAndApplyCLUT(thisMaterial.texturePage, thisMaterial.clutValue, leftCoordInt, rightCoordInt, southCoordInt,
                        northCoordInt, objectName, (thisMaterial.textureID + 1), materialID, j + 1, empty) != 0)
                    { std::cerr << std::format("	Export Error: Level subframe texture {}-tex{}-{}.png failed to export",
                        objectName, (thisMaterial.textureID + 1), (j + 1)) << std::endl; }
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

bool objectSubframePointCorrectionAndExport(unsigned int materialID, unsigned int textureID, std::string objectName,
    ObjectAnimationSubframe subframe)
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
	int texPageReturnValue = goToTexPageAndApplyCLUT(subframe.texturePage, subframe.clutValue, leftCoordInt, rightCoordInt,
        southCoordInt, northCoordInt, objectName, (textureID + 1), materialID, (subframe.subframeID + 1), empty);
	if (texPageReturnValue != 0)
	{
		std::cerr << std::format("	Export Error: Object subframe texture {}-tex{}-{}.png failed to export",
            objectName, (textureID + 1), (subframe.subframeID + 1)) << std::endl;
		return false;
	}
	return true;
}

