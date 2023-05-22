# Gex 2 PS1 Model Exporter
This is a simple command line program written in C++ that exports Gex 2 PS1 model files. This is my first ever C++ program so it's a little rough around the edges and has some novice bits of code, but at the end of the day it works and I'm glad it does.

## Usage
There are 3 parameters in the program, 1 is needed needed and the other 2 are optional.

The 1st parameter is the **input file**. This is the model file from gex 2 (extension is _.drm_). The parameter can be either the local location of the file, relative to the program's location, or the exact location (specified on most OS's as having a forward slash at the start. On windows you put the volume at the start, e.g. C:\)

The 2nd parameter is the **output folder**. This is the folder where the models will be output. Note that this does not create a folder with the name of the parameter; the folder must be preexisting in order to work. If there is no 2nd parameter, it uses the current working directory.

The 3rd parameter is the **model index**. This is the index of whichever model you want to export. The value -1 is reserved for exporting everything. The value 0 is reserved for the level geometry. Any value >0 is used for the several other models stored in the input file. If there is no 3rd parameter, it defaults to exporting everything.

Usage on the command line is as follows:
```
> Gex2PS1ModelExporter.exe [input file] [output folder] [model index]
```

## Getting the Model Files
To get the model files you will need:
* A dumped ROM of Gex: Enter the Gecko on PS1
* A program to open .bin files such as [PowerISO](https://www.poweriso.com)
* [Soul Spiral](https://github.com/TheSerioliOfNosgoth/SoulSpiral-Official)

First, open the Gex 2 .bin file using PowerISO. Extract the file called _BIGFILE.DAT_.

Open Soul Spiral and click on the button "Open a BigFile". Navigate to where _BIGFILE.DAT_ is and select it. Use the automatic settings. Export the DRM files that you want.

## Compilation & Building
This program requires:
* [TinyXML2](https://github.com/leethomason/tinyxml2) to make the DAE files
* [CMake](https://cmake.org) to build
* [Clang](https://clang.llvm.org) to compile
Install CMake if you haven't already and clone the TinyXML2 repository onto your computer.

If you're sticking with using Visual Studio as the CMake generator, go to the Visual Studio Installer and install the _C++ Clang Compiler for Windows_ and _MSBuild support for LLVM (clang-cl) toolset_ found under Individual components.

If you're using an alternate method, modify the CMake files as needed.

### TinyXML2
First, clone the repository into your chosen location. Navigate to the folder _libraries_ and create a new folder there named _tinyxml2_. Go into that folder and create two more folders, one called _include_ and one called _lib_. Go to the _include_ folder and create a new folder named _tinyxml2_.

Navigate to the TinyXML2 repository and copy the files _tinyxml2.cpp_ and _tinyxml2.h_. Navigate back to the model exporter repository and paste them into the _tinyxml2_ folder found in _include_ ([repository]/libraries/tinyxml2/include/tinyxml2).

Next, build the TinyXML2 lib file using CMake (there are plenty of tutorials online on how to use CMake if you are struggling). After you're done building, go to either the _Debug_ or _Release_ folder (depending on the one you built) and copy the file _tinyxml2.lib_.

Navigate back to the model exporter repository and paste the file into the _lib_ folder ([repository]/libraries/tinyxml2/include/lib).

If you have followed these steps correctly, TinyXML2 should now be added as an external library.

### Compile using Clang
If you are using Visual Studio as the CMake generator, build the Visual Studio solution using CMake. Open the generated solution file, go to Build -> Batch Build, and select whichever one you were working with, using _Gex2PS1ModelExporter_ as the project. Click build and the program should be created.

## Compatibility
Currently I have only tested the program on Windows 64-bit, and have only built a Windows release. However I am aiming to make the program cross-compatible with Linux.

## Credits
* Crystal Dynamics for their amazing game
* [TheSerioliOfNosgoth](https://github.com/TheSerioliOfNosgoth) for their work in reverse engineering Crystal Dynamics games
* [The Gex Discord](https://discord.gg/TeA7D4f) for their work in documenting and modding the Gex games