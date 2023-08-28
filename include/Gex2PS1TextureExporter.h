#pragma once

#include "TextureStructs.h"
#include "Gex2PS1SharedFunctions.h"
#include <string>

int goToTexPageAndApplyCLUT(unsigned short int texturePage, unsigned short int clutValue, unsigned int left, unsigned int right, unsigned int south, unsigned int north, std::string objectName, std::string outputFolder, unsigned int textureIndex, unsigned int materialIndex, unsigned int subframe, std::vector<LevelAnimationSubframe>& levelSubframes);
bool resetModifiedVRAM();
int initialiseVRM(std::string path);
int copyRectangleInVRM(unsigned short int xCoordinateDestination, unsigned short int yCoordinateDestination, unsigned short int xSize, unsigned short int ySize, unsigned short int xCoordinateSource, unsigned short int yCoordinateSource, bool useAlreadyModifiedVRAMAsBase);