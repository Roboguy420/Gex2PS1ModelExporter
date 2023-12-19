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

#include <vector>

// "Intermediary" model structures

struct Vertex
{
	int positionID;
	int normalID;
	int boneID;
	int UVID;
	short int rawX, rawY, rawZ;
	short int finalX, finalY, finalZ;
};

struct Bone
{
	unsigned short int vFirst, vLast;
	short int localX, localY, localZ;
	short int worldX, worldY, worldZ;
	unsigned short int parentID;
};

struct UV
{
	float u, v;
};
inline bool sortUCoord(const UV &a, const UV &b)
{
	return a.u < b.u;
}
inline bool sortVCoord(const UV &a, const UV &b)
{
	return a.v < b.v;
}

struct PolygonStruct
{
	Vertex v1, v2, v3;
	unsigned int materialID;
	UV uv1, uv2, uv3;
};

struct ObjectAnimationSubframe
{
	unsigned short int clutValue;
	unsigned short int texturePage;
	std::vector<UV> UVs;
	unsigned int baseMaterialAddress;
	unsigned int subframeID;
};

struct Material
{
	bool realMaterial;
	bool visible; // Only used for level geometry
	bool properlyExported;
	unsigned char redVal;
	unsigned char blueVal;
	unsigned char greenVal;
	unsigned short int clutValue;
	unsigned short int texturePage;
	unsigned int textureID;
	std::vector<ObjectAnimationSubframe> objectSubframes;
};