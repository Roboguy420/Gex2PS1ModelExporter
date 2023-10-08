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

#include "TextureStructs.h"
#include "Gex2PS1SharedFunctions.h"
#include <string>

int goToTexPageAndApplyCLUT(unsigned short int texturePage, unsigned short int clutValue, unsigned int left, unsigned int right, unsigned int south, unsigned int north, std::string objectName, std::string outputFolder, unsigned int textureIndex, unsigned int materialIndex, unsigned int subframe, std::vector<LevelAnimationSubframe>& levelSubframes);
bool resetModifiedVRAM();
int initialiseVRM(std::string path);
int copyRectangleInVRM(unsigned short int xCoordinateDestination, unsigned short int yCoordinateDestination, unsigned short int xSize, unsigned short int ySize, unsigned short int xCoordinateSource, unsigned short int yCoordinateSource, bool useAlreadyModifiedVRAMAsBase);