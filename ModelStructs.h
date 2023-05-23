#pragma once

#include "Gex2PS1ModelExporter.h"

//"Intermediary" model structures

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

struct PolygonStruct
{
	Vertex v1, v2, v3;
	unsigned int materialID;
	UV uv1, uv2, uv3;
};

struct Material
{
	bool realMaterial;
	unsigned short int clutValue;
	unsigned short int texturePage;
	UV uvCoordinates[4];
};