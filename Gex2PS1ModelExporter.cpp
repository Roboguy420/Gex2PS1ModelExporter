#define NOMINMAX

#include <tinyxml2/tinyxml2.h>

#include <Gex2PS1ModelExporter.h>
#include "ModelStructs.h"
#include <Windows.h>
#include <filesystem>
#include <algorithm>
#include <vector>

int convertObjToDAE(ifstreamoffset& reader, std::string outputFolder, std::string objectName, std::string inputFile);

int main(int argc, char* argv[])
{
	if (argc < 2)
	{
		return 1;
		//Needs at least the input file to work
		//The output destination + selected model index are optional parameters
	}
	std::string inputFile = argv[1];
	std::string outputFolder;

	//Selected export -1 = everything
	//Selected export 0 = level geometry
	//Selected export >0 = other object models
	int selectedModelExport = -1;

	char outputFolderBuffer[2048]; //You can modify this value if you're using some other OS that has an even larger directory length.
	_getcwd(outputFolderBuffer, 2048);
	outputFolder = outputFolderBuffer;

	if (argc > 2 && std::filesystem::is_directory(argv[2]))
	{
		outputFolder = argv[2];
		//If the directory in argument 2 does not exist, it ignores it and uses the current working directory
	}
	if (argc > 3)
	{
		selectedModelExport = stringToInt(argv[3], -1);
	}

	if (!std::filesystem::exists(inputFile))
	{
		//Input file doesn't exist
		return 2;
	}

	ifstreamoffset reader(inputFile, std::ifstream::binary);
	reader.exceptions(ifstreamoffset::eofbit);

	if (!reader)
	{
		return 3;
	}

	if (!std::filesystem::exists(outputFolder))
	{
		//Failed to access output folder
		return 4;
	}

	outputFolder = outputFolder + directorySeparator() + getFileNameWithoutExtension(inputFile, false);

	int returnCode = 0;
	bool atLeastOneExportedSuccessfully = false;

	try
	{
		initialiseVRM(std::format("{}.vrm", getFileNameWithoutExtension(inputFile, true)));
		unsigned int bitshift;
		reader.read((char*)&bitshift, sizeof(bitshift));
		bitshift = ((bitshift >> 9) << 11) + 0x800;
		reader.superOffset = bitshift;

		unsigned int modelsAddressesStart;
		reader.seekg(0x3C, reader.beg);
		reader.read((char*)&modelsAddressesStart, sizeof(modelsAddressesStart));
		reader.seekg(modelsAddressesStart, reader.beg);

		unsigned int objIndex = 0;
		while (selectedModelExport != 0)
		{
			unsigned int specificObjectAddress;
			reader.read((char*)&specificObjectAddress, sizeof(specificObjectAddress));

			if (specificObjectAddress == modelsAddressesStart)
			{
				break;
			}

			objIndex += 1;

			long int nextPos = reader.tellg();

			if (objIndex == selectedModelExport || selectedModelExport == -1)
			{
				reader.seekg(specificObjectAddress + 0x24, reader.beg);
				unsigned int objNameAddr;
				reader.read((char*)&objNameAddr, sizeof(objNameAddr));
				reader.seekg(objNameAddr, reader.beg);
				std::string objName;
				for (int i = 0; i < 8; i++)
				{
					char objNameChar;
					reader.read((char*)&objNameChar, 1);
					objName += objNameChar;
				}

				unsigned short int objectCount;
				unsigned int objectStartAddress;
				reader.seekg(specificObjectAddress + 0x8, reader.beg);
				reader.read((char*)&objectCount, sizeof(objectCount));
				reader.seekg(2, reader.cur);
				reader.read((char*)&objectStartAddress, sizeof(objectStartAddress));

				for (int i = 0; i < objectCount; i++)
				{
					reader.seekg(objectStartAddress + (i * 4), reader.beg);
					unsigned int objectModelData;
					reader.read((char*)&objectModelData, sizeof(objectModelData));

					std::string objectNameAndIndex = objName;
					if (objectCount > 1)
					{
						objectNameAndIndex = objName + std::to_string(i + 1);
					}

					reader.seekg(objectModelData, reader.beg);
					int objectReturnCode = convertObjToDAE(reader, outputFolder, objectNameAndIndex, inputFile);
					if (returnCode != 6 && objectReturnCode == 1)
					{
						//At least 1 texture failed to export
						returnCode = 6;
					}

					if (returnCode != 7 && objectReturnCode == 2)
					{
						//Model failed to export
						returnCode = 7;
					}
					else
					{
						atLeastOneExportedSuccessfully = true;
					}
				}
				if (objIndex == selectedModelExport) { break; }
			}

			reader.seekg(nextPos, reader.beg);
		}
	}
	catch (ifstreamoffset::failure e)
	{
		return 5;
	}
	if (!atLeastOneExportedSuccessfully)
	{
		return 8;
	}
	return returnCode;
}

void readArmature(ifstreamoffset &reader, unsigned short int boneCount, unsigned int boneStartAddress, std::vector<Bone>& bones)
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

void applyArmature(ifstreamoffset& reader, unsigned short int vertexCount, unsigned int vertexStartAddress, unsigned short int boneCount, unsigned int boneStartAddress, std::vector<Vertex>& vertices, std::vector<Bone>& bones)
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

Vertex readVertex(ifstreamoffset &reader, unsigned int v)
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

void readVertices(ifstreamoffset &reader, unsigned short int vertexCount, unsigned int vertexStartAddress, unsigned short int boneCount, unsigned int boneStartAddress, std::vector<Vertex>& vertices)
{
	if (vertexStartAddress == 0 || vertexCount == 0) { return; }

	reader.seekg(vertexStartAddress, reader.beg);

	for (unsigned int v = 0; v < vertexCount; v++)
	{
		unsigned int uPolygonPosition = reader.tellg();
		vertices.push_back(readVertex(reader, v));
		reader.seekg(uPolygonPosition + 0xC, reader.beg);
	}

	std::vector<Bone> bones;

	readArmature(reader, boneCount, boneStartAddress, bones);

	applyArmature(reader, vertexCount, vertexStartAddress, boneCount, boneStartAddress, vertices, bones);
}

float* UVPointCorrection(byte v1U, byte v1V, byte v2U, byte v2V, byte v3U, byte v3V)
{
	//!!!!!!!!!!!!!!!!!!!!!!
	//!	 				   !
	//!  Currently unused  !
	//!                    !
	//!!!!!!!!!!!!!!!!!!!!!!

	float uAxis[3];
	float vAxis[3];
	uAxis[0] = v1U;
	vAxis[0] = v1V;
	uAxis[1] = v2U;
	vAxis[1] = v2V;
	uAxis[2] = v3U;
	vAxis[2] = v3V;

	unsigned int highestPointIndex = getMinOrMaxIndexOfThree(vAxis[0], vAxis[1], vAxis[2], false);
	unsigned int lowestPointIndex = getMinOrMaxIndexOfThree(vAxis[0], vAxis[1], vAxis[2], true);
	unsigned int farRightPointIndex = getMinOrMaxIndexOfThree(uAxis[0], uAxis[1], uAxis[2], false);
	unsigned int farLeftPointIndex = getMinOrMaxIndexOfThree(uAxis[0], uAxis[1], uAxis[2], true);

	unsigned int hingeIndex;

	if (highestPointIndex == farRightPointIndex || highestPointIndex == farLeftPointIndex)
	{
		hingeIndex = highestPointIndex;
	}
	else
	{
		hingeIndex = lowestPointIndex;
	}

	uAxis[farRightPointIndex] = 255.0f;
	uAxis[farLeftPointIndex] = 0.0f;
	vAxis[highestPointIndex] = 255.0f;
	vAxis[lowestPointIndex] = 0.0f;

	if (hingeIndex == highestPointIndex && hingeIndex == farLeftPointIndex)
	{
		uAxis[hingeIndex] = 0.0f;
		vAxis[hingeIndex] = 255.0f;
		vAxis[farRightPointIndex] = 255.0f;
		uAxis[lowestPointIndex] = 0.0f;
	}
	else if (hingeIndex == highestPointIndex && hingeIndex == farRightPointIndex)
	{
		uAxis[hingeIndex] = 255.0f;
		vAxis[hingeIndex] = 255.0f;
		vAxis[farLeftPointIndex] = 255.0f;
		uAxis[lowestPointIndex] = 255.0f;
	}
	else if (hingeIndex == lowestPointIndex && hingeIndex == farLeftPointIndex)
	{
		uAxis[hingeIndex] = 0.0f;
		vAxis[hingeIndex] = 0.0f;
		vAxis[farRightPointIndex] = 0.0f;
		uAxis[highestPointIndex] = 0.0f;
	}
	else
	{
		uAxis[hingeIndex] = 255.0f;
		vAxis[hingeIndex] = 0.0f;
		vAxis[farLeftPointIndex] = 0.0f;
		uAxis[highestPointIndex] = 255.0f;
	}

	float newUVCoords[6] = {uAxis[0], vAxis[0], uAxis[1], vAxis[1], uAxis[2], vAxis[2]};

	return newUVCoords;
}

Material readMaterial(ifstreamoffset &reader, unsigned int p, std::vector<Material> materials)
{
	Material thisMaterial;
	thisMaterial.realMaterial = true;
	thisMaterial.properlyExported = true;

	byte u[4];
	byte v[4];

	reader.read((char*)&u[0], 1);
	reader.read((char*)&v[0], 1);
	reader.read((char*)&thisMaterial.clutValue, sizeof(thisMaterial.clutValue));
	reader.read((char*)&u[1], 1);
	reader.read((char*)&v[1], 1);
	reader.read((char*)&thisMaterial.texturePage, sizeof(thisMaterial.texturePage));
	reader.read((char*)&u[2], 1);
	reader.read((char*)&v[2], 1);

	float sideA = sqrt(pow(u[1] - u[2], 2) + pow(v[1] - v[2], 2));
	float sideB = sqrt(pow(u[0] - u[2], 2) + pow(v[0] - v[2], 2));
	float sideC = sqrt(pow(u[0] - u[1], 2) + pow(v[0] - v[1], 2));
	
	unsigned int hingeIndex = getMinOrMaxIndexOfThree(sideA, sideB, sideC, false);

	int notHinge[2];

	int notHingeIterator = 0;
	for (int i = 0; i < 3; i++)
	{
		if (i != hingeIndex)
		{
			notHinge[notHingeIterator] = i;
			notHingeIterator++;
		}
	}

	u[3] = u[notHinge[0]] + u[notHinge[1]] - u[hingeIndex];
	v[3] = v[notHinge[0]] + v[notHinge[1]] - v[hingeIndex];

	thisMaterial.uvCoordinates[0].u = u[0];
	thisMaterial.uvCoordinates[0].v = 255 - v[0];
	thisMaterial.uvCoordinates[1].u = u[1];
	thisMaterial.uvCoordinates[1].v = 255 - v[1];
	thisMaterial.uvCoordinates[2].u = u[2];
	thisMaterial.uvCoordinates[2].v = 255 - v[2];
	thisMaterial.uvCoordinates[3].u = u[3];
	thisMaterial.uvCoordinates[3].v = 255 - v[3];

	return thisMaterial;
}

float* readUVCoordinates(ifstreamoffset &reader)
{
	//!!!!!!!!!!!!!!!!!!!!!!
	//!	 				   !
	//!  Currently unused  !
	//!                    !
	//!!!!!!!!!!!!!!!!!!!!!!

	byte v1U;
	byte v1V;
	byte v2U;
	byte v2V;
	byte v3U;
	byte v3V;

	reader.read((char*)&v1U, 1);
	reader.read((char*)&v1V, 1);
	reader.seekg(2, reader.cur);
	reader.read((char*)&v2U, 1);
	reader.read((char*)&v2V, 1);
	reader.seekg(2, reader.cur);
	reader.read((char*)&v3U, 1);
	reader.read((char*)&v3V, 1);

	float* newUVCoords;

	newUVCoords = UVPointCorrection(v1U, v1V, v2U, v2V, v3U, v3V);

	return newUVCoords;
}

PolygonStruct readPolygon(ifstreamoffset& reader, std::string objectName, std::string outputFolder, unsigned int p, std::vector<Material>& materials, std::vector<Vertex>& vertices)
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

	reader.seekg(1, reader.cur);

	byte polygonFlags;
	reader.read((char*)&polygonFlags, sizeof(polygonFlags));

	if ((polygonFlags & 0x02) == 0x02)
	{
		unsigned int materialPosition;
		reader.read((char*)&materialPosition, sizeof(materialPosition));

		reader.seekg(materialPosition, reader.beg);

		byte u[3];
		byte v[3];
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

		reader.seekg(materialPosition, reader.beg);
		Material thisMaterial = readMaterial(reader, p, materials);

		bool newMaterial = true;
		for (int m = 0; m < materials.size(); m++)
		{
			bool newClutValue = true;
			bool newTexturePage = true;
			bool newUVCoordinates[4];
			newUVCoordinates[0] = true;
			newUVCoordinates[1] = true;
			newUVCoordinates[2] = true;
			newUVCoordinates[3] = true;
			if (thisMaterial.clutValue == materials[m].clutValue)
			{
				newClutValue = false;
			}
			if (thisMaterial.texturePage == materials[m].texturePage)
			{
				newTexturePage = false;
			}

			for (int thisMaterialUV = 0; thisMaterialUV < 4; thisMaterialUV++)
			{
				for (int iteratorMaterialUV = 0; iteratorMaterialUV < 4; iteratorMaterialUV++)
				{
					if (thisMaterial.uvCoordinates[thisMaterialUV].u == materials[m].uvCoordinates[iteratorMaterialUV].u
						&& thisMaterial.uvCoordinates[thisMaterialUV].v == materials[m].uvCoordinates[iteratorMaterialUV].v)
					{
						newUVCoordinates[thisMaterialUV] = false;
					}
				}
			}

			if (!newClutValue && !newTexturePage) //&& !newUVCoordinates[0] && !newUVCoordinates[1] && !newUVCoordinates[2] && !newUVCoordinates[3]
			{
				newMaterial = false;
				thisPolygon.materialID = m;
				break;
			}
		}
		if (newMaterial)
		{
			unsigned int textureCount = 0;
			for (int i = 0; i < materials.size(); i++)
			{
				if (materials[i].realMaterial)
				{
					textureCount++;
				}
			}
			thisMaterial.textureID = textureCount;
			int texPageReturnValue = goToTexPageAndApplyCLUT(thisMaterial.texturePage, thisMaterial.clutValue, objectName, outputFolder, (thisMaterial.textureID + 1));
			if (texPageReturnValue != 0)
			{
				thisMaterial.properlyExported = false;
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
			if (!materials[m].realMaterial)
			{
				newMaterial = false;
				thisPolygon.materialID = m;
				break;
			}
		}
		if (newMaterial)
		{
			Material fakeMaterial;
			fakeMaterial.realMaterial = false;
			fakeMaterial.properlyExported = true;
			materials.push_back(fakeMaterial);
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

void readPolygons(ifstreamoffset& reader, std::string objectName, std::string outputFolder, unsigned short int polygonCount, unsigned int polygonStartAddress, std::vector<PolygonStruct>& polygons, std::vector<Material>& materials, std::vector<Vertex>& vertices)
{
	if (polygonStartAddress == 0 || polygonCount == 0) { return; }

	reader.seekg(polygonStartAddress, reader.beg);

	for (unsigned short int p = 0; p < polygonCount; p++)
	{
		unsigned int uPolygonPosition = reader.tellg();
		polygons.push_back(readPolygon(reader, objectName, outputFolder, p, materials, vertices));
		reader.seekg(uPolygonPosition + 0xC, reader.beg);
	}
}

int convertObjToDAE(ifstreamoffset &reader, std::string outputFolder, std::string objectName, std::string inputFile)
{
	int returnValue = 0;

	unsigned short int vertexCount;
	unsigned int vertexStartAddress;
	unsigned short int polygonCount;
	unsigned int polygonStartAddress;
	unsigned short int boneCount;
	unsigned int boneStartAddress;
	unsigned int materialStartAddress;

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
	reader.read((char*)&materialStartAddress, sizeof(materialStartAddress));

	std::vector<Vertex> vertices;

	readVertices(reader, vertexCount, vertexStartAddress, boneCount, boneStartAddress, vertices);

	std::vector<PolygonStruct> polygons;
	std::vector<Material> materials;

	readPolygons(reader, objectName, outputFolder, polygonCount, polygonStartAddress, polygons, materials, vertices);

	unsigned int oldPosition = reader.tellg();

	//Tried using ASSIMP but there was a severe lack of tutorials on how to create a scene from scratch. Pretty much everything online focused solely on importing.
	//So instead, here is my attempt at creating a DAE file using tinyxml2. Enjoy...

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
			init_fromDiffuseImage->SetText(std::format("{}-tex{}.png", objectName, materials[m].textureID+1).c_str());
			image->LinkEndChild(init_fromDiffuseImage);
			library_images->LinkEndChild(image);
		}

		//Effects

		if (materials[m].realMaterial && materials[m].properlyExported)
		{
			tinyxml2::XMLElement* effect = outputDAE.NewElement("effect");
			effect->SetAttribute("id", std::format("{}-{}-fx", objectName, materials[m].textureID).c_str());
			effect->SetAttribute("name", std::format("{}-{}", objectName, materials[m].textureID).c_str());
			tinyxml2::XMLElement* profile_COMMON = outputDAE.NewElement("profile_COMMON");

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

			tinyxml2::XMLElement* technique = outputDAE.NewElement("technique");
			technique->SetAttribute("sid", "standard");
			tinyxml2::XMLElement* phong = outputDAE.NewElement("phong");
			tinyxml2::XMLElement* diffuse = outputDAE.NewElement("diffuse");
			tinyxml2::XMLElement* texture = outputDAE.NewElement("texture");
			texture->SetAttribute("texture", std::format("{}-{}-diffuse-sampler", objectName, materials[m].textureID).c_str());
			texture->SetAttribute("texcoord", "CHANNEL0");
			diffuse->LinkEndChild(texture);
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
		if (materials[m].realMaterial && materials[m].properlyExported)
		{
			tinyxml2::XMLElement* instance_effectMaterial = outputDAE.NewElement("instance_effect");
			instance_effectMaterial->SetAttribute("url", std::format("#{}-{}-fx", objectName, materials[m].textureID).c_str());
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
		coloursFloat_array->SetAttribute("id", std::format("meshId{}-color-array", m).c_str());
		coloursFloat_array->SetAttribute("count", meshPolygons.size() * 9);
		std::string coloursString = " ";
		for (int c = 0; c < meshPolygons.size() * 9; c++)
		{
			//Temporary thing as I'm not sure how colours work in the models yet
			coloursString += "0 ";
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

		tinyxml2::XMLElement *xVertices = outputDAE.NewElement("vertices");
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
			polyString += std::format("{} {} {} ", (p*3), (p*3) + 1, (p*3) + 2);
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

	std::filesystem::create_directory(outputFolder);
	if (std::filesystem::exists(outputFolder))
	{
		outputDAE.SaveFile(std::format("{}{}{}.dae", outputFolder, directorySeparator(), objectName).c_str());
		return returnValue;
	}

	//Could not export
	return 2;
}