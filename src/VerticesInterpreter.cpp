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

#include <fstream>

#include "VerticesInterpreter.h"

void readVertices(std::ifstream& reader, unsigned short int vertexCount, unsigned int vertexStartAddress, unsigned short int boneCount,
    unsigned int boneStartAddress, bool isObject, std::vector<Vertex>& vertices)
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

void applyArmature(std::ifstream& reader, unsigned short int vertexCount, unsigned int vertexStartAddress, unsigned short int boneCount,
    unsigned int boneStartAddress, std::vector<Vertex>& vertices, std::vector<Bone>& bones)
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