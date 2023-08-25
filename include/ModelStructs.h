#pragma once

#include <vector>

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
static bool sortUCoord(const UV &a, const UV &b)
{
	return a.u < b.u;
}
static bool sortVCoord(const UV &a, const UV &b)
{
	return a.v < b.v;
}

struct PolygonStruct
{
	Vertex v1, v2, v3;
	unsigned int materialID;
	UV uv1, uv2, uv3;
};

struct LevelAnimationSubframe
{
	unsigned short int xCoordinateDestination;
	unsigned short int yCoordinateDestination;
	unsigned short int xSize;
	unsigned short int ySize;
	std::vector<unsigned short int> xCoordinateSources;
	std::vector<unsigned short int> yCoordinateSources;
	bool subframeExportsThis;
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
	bool visible; //Only used for level geometry
	bool properlyExported;
	unsigned short int clutValue;
	unsigned short int texturePage;
	unsigned int textureID;
	std::vector<ObjectAnimationSubframe> objectSubframes;
};