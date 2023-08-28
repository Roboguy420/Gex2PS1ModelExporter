#pragma once

#include <vector>

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