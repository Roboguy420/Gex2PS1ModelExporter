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

#include <iostream>

void readVertices(std::ifstream& reader, unsigned short int vertexCount, unsigned int vertexStartAddress, unsigned short int boneCount,
    unsigned int boneStartAddress, bool isObject, std::vector<Vertex>& vertices);

Vertex readVertex(std::ifstream& reader, unsigned int v);

void readArmature(std::ifstream& reader, unsigned short int boneCount, unsigned int boneStartAddress, std::vector<Bone>& bones);

void applyArmature(std::ifstream& reader, unsigned short int vertexCount, unsigned int vertexStartAddress, unsigned short int boneCount,
    unsigned int boneStartAddress, std::vector<Vertex>& vertices, std::vector<Bone>& bones);