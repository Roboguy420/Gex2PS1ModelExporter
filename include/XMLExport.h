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

#include <iostream>
#include <vector>
#include "ModelStructs.h"
#include "tinyxml2.h"

int exportToXML(std::string outputFolder, std::string objectName, std::vector<PolygonStruct>& polygons, std::vector<Material>& materials);

int exportTexture(tinyxml2::XMLDocument& outputDAE, tinyxml2::XMLElement* library_images, Material exportMaterial, std::string objectName);

int exportEffect(tinyxml2::XMLDocument& outputDAE, tinyxml2::XMLElement* library_effects, Material exportMaterial, int materialID, std::string objectName);

int exportMaterial(tinyxml2::XMLDocument& outputDAE, tinyxml2::XMLElement* material, tinyxml2::XMLElement* library_materials,
    Material exportMaterial, int materialID, std::string objectName);

int exportGeometry(tinyxml2::XMLDocument& outputDAE, std::vector<PolygonStruct>& polygons, tinyxml2::XMLElement* geometry, tinyxml2::XMLElement* mesh,
    tinyxml2::XMLElement* library_geometries, std::vector<PolygonStruct>& meshPolygons, std::vector<Vertex>& meshVertices, Material exportMaterial, int materialID,
    std::string objectName);

int exportPositions(tinyxml2::XMLDocument& outputDAE, tinyxml2::XMLElement* mesh,
    std::vector<PolygonStruct>& meshPolygons, std::vector<Vertex>& meshVertices, int materialID);

int exportTextures(tinyxml2::XMLDocument& outputDAE, tinyxml2::XMLElement* mesh, std::vector<PolygonStruct>& meshPolygons, int materialID);

int exportColours(tinyxml2::XMLDocument& outputDAE, tinyxml2::XMLElement* mesh, std::vector<PolygonStruct>& meshPolygons, Material exportMaterial, int materialID);

int exportVertices(tinyxml2::XMLDocument& outputDAE, tinyxml2::XMLElement* mesh, int materialID);

int exportPolygons(tinyxml2::XMLDocument& outputDAE, tinyxml2::XMLElement* mesh, unsigned int meshPolygonsSize, int materialID);

int exportVisualScene(tinyxml2::XMLDocument& outputDAE, tinyxml2::XMLElement* nodeModel, int materialID, std::string objectName);
