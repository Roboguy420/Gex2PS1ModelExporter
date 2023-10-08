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

#include "png.h"

#include "Gex2PS1TextureExporter.h"

#include <filesystem>
#include <format>

unsigned short int** textureData = new unsigned short int* [512];
unsigned short int** textureDataVRAMMovement = new unsigned short int* [512];

int initialiseVRM(std::string path)
{
	std::ifstream reader(path, std::ifstream::binary);
	reader.seekg(20, reader.beg);

	for (int y = 0; y < 512; y++)
	{
		textureData[y] = new unsigned short int[512];
	}

	for (int y = 0; y < 512; y++)
	{
		for (int x = 0; x < 512; x++)
		{
			reader.read((char*)&(textureData[y][x]), 2);
		}
	}

	for (int y = 0; y < 512; y++)
	{
		textureDataVRAMMovement[y] = new unsigned short int[512];
	}

	resetModifiedVRAM();

	return 0;
}

bool resetModifiedVRAM()
{
	for (unsigned int y = 0; y < 512; y++)
	{
		for (unsigned int x = 0; x < 512; x++)
		{
			textureDataVRAMMovement[y][x] = textureData[y][x];
		}
	}

	return true;
}

int copyRectangleInVRM(unsigned short int xCoordinateDestination, unsigned short int yCoordinateDestination, unsigned short int xSize, unsigned short int ySize, unsigned short int xCoordinateSource, unsigned short int yCoordinateSource, bool useAlreadyModifiedVRAMAsBase)
{
	for (unsigned int y = 0; y < ySize; y++)
	{
		for (unsigned int x = 0; x < xSize; x++)
		{
			if (!useAlreadyModifiedVRAMAsBase)
				resetModifiedVRAM();

			textureDataVRAMMovement[yCoordinateDestination + y][xCoordinateDestination + x] = textureDataVRAMMovement[yCoordinateSource + y][xCoordinateSource + x];
		}
	}

	return 0;
}

int goToTexPageAndApplyCLUT(unsigned short int texturePage, unsigned short int clutValue, unsigned int left, unsigned int right, unsigned int south, unsigned int north, std::string objectName, std::string outputFolder, unsigned int textureIndex, unsigned int materialIndex, unsigned int subframe, std::vector<LevelAnimationSubframe>& levelSubframes)
{
	//Initialise texture page

	int texturePageX = (texturePage << 6) & 0x07C0;
	int texturePageY = (texturePage << 4) & 0x0100 + ((texturePage >> 2) & 0x0200);
	texturePageX %= 512;
	texturePageX += 512;
	texturePageX %= 512;
	unsigned short int** pixels = new unsigned short int*[256];
	unsigned int colourLimit = 16;

	for (int y = 0; y < 256; y++)
	{
		pixels[y] = new unsigned short int[256];
	}

	bool* subframeCheckAlreadyDone = new bool[levelSubframes.size()];

	for (unsigned int i = 0; i < levelSubframes.size(); i++)
	{
		subframeCheckAlreadyDone[i] = false;
	}

	for (int y = 0; y < 256; y++)
	{
		for (int x = 0; x < 256;)
		{
			int wrappedWidth;
			if (((texturePage >> 7) & 0x3) == 0) // 4 bit
			{
				colourLimit = 16;
				unsigned short int val = 0;
				wrappedWidth = (texturePageX + (x / 4)) % 512;
				if ((texturePageY + y) < 512)
					val = textureDataVRAMMovement[texturePageY + y][wrappedWidth];

				pixels[y][x++] = val & 0x000F;
				pixels[y][x++] = (val & 0x00F0) >> 4;
				pixels[y][x++] = (val & 0x0F00) >> 8;
				pixels[y][x] = (val & 0xF000) >> 12;
			}
			else if (((texturePage >> 7) & 0x3) == 1) // 8 bit
			{
				colourLimit = 256;
				unsigned short int val = 0;
				wrappedWidth = (texturePageX + (x / 2)) % 512;
				if ((texturePageY + y) < 512)
					val = textureDataVRAMMovement[texturePageY + y][wrappedWidth];

				pixels[y][x++] = val & 0x00FF;
				pixels[y][x] = (val & 0xFF00) >> 8;
			}
			else if (((texturePage >> 7) & 0x3) == 2) // 16 bit
			{
				colourLimit = 65536;
				unsigned short int val = 0;
				wrappedWidth = (texturePageX + x) % 512;
				if ((texturePageY + y) < 512)
					val = textureDataVRAMMovement[texturePageY + y][wrappedWidth];

				pixels[y][x] = val;
			}

			if (x >= left && x < right && y >= north && y < south)
			{
				for (unsigned int i = 0; i < levelSubframes.size(); i++)
				{
					if (!subframeCheckAlreadyDone[i] && wrappedWidth >= levelSubframes[i].xCoordinateDestination && wrappedWidth < (levelSubframes[i].xCoordinateDestination + levelSubframes[i].xSize)
						&& (texturePageY + y) >= levelSubframes[i].yCoordinateDestination && (texturePageY + y) < (levelSubframes[i].yCoordinateDestination + levelSubframes[i].ySize))
					{
						levelSubframes[i].subframeExportsThis = true;
						subframeCheckAlreadyDone[i] = true;
					}
				}
			}
			x++;
		}
	}

	free(subframeCheckAlreadyDone);

	//Initialise CLUT

	int colourTableX = (clutValue & 0x3F) << 4;
	int colourTableY = clutValue >> 6;
	colourTableX %= 512;
	colourTableX += 512;
	colourTableX %= 512;
	std::vector<unsigned int> colours;

	for (int x = 0; x < colourLimit; x++)
	{
		unsigned short int val = 0;
		int wrappedWidth = (colourTableX + x) % 512;
		if (colourTableY < 512)
		{
			val = textureDataVRAMMovement[colourTableY][wrappedWidth];
		}

		unsigned short int alpha = val >> 15;
		unsigned short int blue = (((val << 1) >> 11) << 3);
		unsigned short int green = (((val << 6) >> 11) << 3);
		unsigned short int red = (((val << 11) >> 11) << 3);

		alpha &= 0xFF;
		blue &= 0xFF;
		green &= 0xFF;
		red &= 0xFF;

		if (alpha != 0 || blue != 0 || green != 0 || red != 0)
		{
			alpha = 255;
		}
		else
		{
			alpha = 0;
		}

		colours.push_back(alpha * 0x1000000 + red * 0x10000 + green * 0x100 + blue);
	}

	//Write to file

	png_structp pngPointer = png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
	png_infop infoPointer = png_create_info_struct(pngPointer);
	png_byte** rowPointers = NULL;

	png_set_IHDR(pngPointer, infoPointer, (right - left + 1), (south - north + 1), 8, PNG_COLOR_TYPE_RGBA, PNG_INTERLACE_NONE, PNG_COMPRESSION_TYPE_DEFAULT, PNG_FILTER_TYPE_DEFAULT);

	rowPointers = static_cast<png_byte**>(png_malloc(pngPointer, (south - north + 1) * sizeof(png_byte*)));

	for (unsigned int y = north; y <= south; y++)
	{
		png_byte* row = static_cast<png_byte*>(png_malloc(pngPointer, (right - left + 1) * 4));
		rowPointers[y - north] = row;
		for (unsigned int x = left; x <= right; x++)
		{
			int pixel = pixels[y][x];
			*row++ = (colours[pixel] & 0x00FF0000) >> 16;
			*row++ = (colours[pixel] & 0x0000FF00) >> 8;
			*row++ = (colours[pixel] & 0x000000FF);
			*row++ = (colours[pixel] & 0xFF000000) >> 24;
		}
	}

	FILE* writeFile;

	std::string textureIndexString = std::format("{}", textureIndex);
	if (subframe > 0)
	{
		textureIndexString += std::format("-{}", subframe);
	}

	writeFile = fopen(std::format("{}{}{}-tex{}.png", outputFolder, directorySeparator(), objectName, textureIndexString).c_str(), "wb");
	if (!writeFile)
	{
		for (int y = 0; y < 256; y++)
		{
			delete (pixels[y]);
		}
		png_destroy_write_struct(&pngPointer, &infoPointer);
		return 1;
	}

	png_init_io(pngPointer, writeFile);
	png_set_rows(pngPointer, infoPointer, rowPointers);
	png_write_png(pngPointer, infoPointer, PNG_TRANSFORM_IDENTITY, NULL);

	fclose(writeFile);

	for (int y = 0; y < 256; y++)
	{
		delete (pixels[y]);
	}
	png_destroy_write_struct(&pngPointer, &infoPointer);

	return 0;
}