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

#define EXIT_SUCCESSFUL_EXPORT 0
#define EXIT_INSUFFICIENT_ARGS 1
#define EXIT_BAD_ARGS 2
#define EXIT_INPUT_NOT_FOUND 3
#define EXIT_INPUT_FAILED_READ 4
#define EXIT_OUTPUT_NOT_FOUND 5
#define EXIT_INDEX_FAILED_PARSE 6
#define EXIT_END_OF_STREAM 7
#define EXIT_TEMPFILE_FAILED_WRITE 8
#define EXIT_TEMPFILE_FAILED_READ 9
#define EXIT_SOME_TEXTURES_FAILED_EXPORT 10
#define EXIT_SOME_MODELS_FAILED_EXPORT 11
#define EXIT_ALL_MODELS_FAILED_EXPORT 12
