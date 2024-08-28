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

#include <string>
#include <vector>

void readPolygons(std::string objectName, std::string outputFolder, unsigned short int polygonCount,
    unsigned int polygonStartAddress, unsigned int textureAnimationsStartAddress, bool isObject, std::vector<PolygonStruct>& polygons,
    std::vector<Material>& materials, std::vector<Vertex>& vertices);

PolygonStruct readPolygon(unsigned int p, int materialStartAddress, bool isObject,
    std::vector<Material>& materials, std::vector<Vertex>& vertices, std::vector<ObjectAnimationSubframe>& subframes);

void readObjectPolygon(PolygonStruct& thisPolygon, Material& thisMaterial, bool& realMaterial, unsigned int& materialAddress);

void readLevelPolygon(PolygonStruct& thisPolygon, Material& thisMaterial, bool& realMaterial, unsigned int& materialAddress);

Material readMaterial();

std::vector<ObjectAnimationSubframe> readObjectAnimationSubFrames(unsigned int textureAnimationsStartAddress);

ObjectAnimationSubframe readObjectAnimationSubFrame(unsigned int baseMaterialAddress);

std::vector<LevelAnimationSubframe> readLevelAnimationSubFrames(unsigned int textureAnimationsStartAddress);

LevelAnimationSubframe* readLevelAnimationSubFrame(unsigned int baseMaterialAddress);

bool UVPointCorrectionAndExport(unsigned int materialID, bool isObject, std::string objectName, std::string outputFolder, Material thisMaterial,
    std::vector<PolygonStruct>& polygons, bool exportLevelAnimations, std::vector<LevelAnimationSubframe>& levelSubframes);

bool objectSubframePointCorrectionAndExport(unsigned int materialID, unsigned int textureID, std::string objectName,
    std::string outputFolder, ObjectAnimationSubframe subframe);
