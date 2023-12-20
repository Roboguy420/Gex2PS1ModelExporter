﻿/*  Gex2PS1ModelExporter: Command line program for exporting Gex 2 PS1 models
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

#include <string>

int readFile(std::ifstream& reader, std::string inputFile, std::string outputFolder, int selectedModelExport, bool listNamesBool,
	bool& modelFailedToExport, bool& textureFailedToExport, bool& atLeastOneExportedSuccessfully);

int convertObjToDAE(std::ifstream& reader, std::string outputFolder, std::string objectName, std::string inputFile);

int convertLevelToDAE(std::ifstream& reader, std::string outputFolder, std::string inputFile);