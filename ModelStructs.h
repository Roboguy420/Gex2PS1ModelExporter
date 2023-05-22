#pragma once

#include "Gex2PS1ModelExporter.h"

//"Intermediary" model structures that can't directly be put into Assimp

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

struct PolygonStruct
{
	Vertex v1, v2, v3;
	unsigned int materialID;
};

struct Material
{
	unsigned int materialPosition;
	unsigned short int clutValue;
	unsigned short int texturePage;
	float v1U, v1V, v2U, v2V, v3U, v3V;
};