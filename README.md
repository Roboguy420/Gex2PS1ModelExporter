# Gex 2 PS1 Model Exporter
This is a simple command line program written in C++ that exports Gex 2 PS1 model files to COLLADA .dae model files. This is my first ever C++ program so it's a little rough around the edges and has some novice bits of code, but at the end of the day it works and I'm glad it does.

## Download
Binaries can be found in the [releases](https://github.com/Roboguy420/Gex2PS1ModelExporter/releases). Download the latest release for your OS to get all the latest features.

## Usage
### Model Exporter
There are 3 parameters in the program, 1 is needed and the other 2 are optional.

The 1st parameter is the **input file**. This is the model file from Gex 2 (extension is _.drm_). The parameter can be either the local location of the file, relative to the current working directory, or the exact location (specified on most OS's as having a forward slash at the start. On windows you put the volume at the start, e.g. C:\\)

The 2nd parameter is the **output folder**. This is the folder where the models will be output. Note that this does not create a folder with the name of the parameter; the folder must be preexisting in order to work. If there is no 2nd parameter, it uses the current working directory.

The 3rd parameter is the **model index**. This is the index of whichever model you want to export. The value -1 is reserved for exporting everything. The value 0 is reserved for the level geometry (currently unimplemented so it exports nothing). Any value >0 is used for the several other models stored in the input file. If there is no 3rd parameter, it defaults to exporting everything.

Usage on the command line is as follows:
```
> Gex2PS1ModelExporter.exe [input file] [output folder] [model index]
```

### Model Names Lister
There is only 1 parameter in the program, which is the **input file**. This is the model file from Gex 2 (extension is _.drm_). The parameter can be either the local location of the file, relative to the current working directory, or the exact location (specified on most OS's as having a forward slash at the start. On windows you put the volume at the start, e.g. C:\\)

Usage on the command line is as follows:
```
> Gex2PS1ModelNamesLister.exe [input file]
```

## Getting the Model Files
To get the model files you will need:
* A dumped ROM of Gex: Enter the Gecko on PS1
* A program to open .bin files such as [PowerISO](https://www.poweriso.com)
* [Soul Spiral](https://github.com/TheSerioliOfNosgoth/SoulSpiral-Official)

First, open the Gex 2 .bin file using PowerISO. Extract the file called _BIGFILE.DAT_.

Open Soul Spiral and click on the button "Open a BigFile". Navigate to where _BIGFILE.DAT_ is and select it. Use the automatic settings. Export the DRM files that you want, alongside their respective VRM files.

## Return Values
The return values are as follows:
* **0:** Successful export with no errors
* **1:** Insufficient arguments
* **2:** Input file not found
* **3:** Failed to read input file
* **4:** Output folder not found/failed to write to
* **5:** End of stream exception (usually the result of a corrupt or unusually formatted DRM file)
* **6:** At least 1 texture failed to export - You are likely to get this a lot because one of etvbutn_2's textures does not export properly. This object is in virtually all the game's model files
* **7:** At least 1 successful export, others had failures
* **8:** No successful exports, all attempts failed

## Compilation & Building
This program requires:
* [TinyXML2](https://github.com/leethomason/tinyxml2) to make the DAE files
* [libpng](https://sourceforge.net/projects/libpng/files/) to export textures to PNG
* [zlib](https://sourceforge.net/projects/libpng/files/zlib/) to compile libpng
* [CMake](https://cmake.org) to build
* [Clang](https://clang.llvm.org) to compile

Install CMake if you haven't already and clone the TinyXML2 repository onto your computer.

If you're sticking with using Visual Studio as the CMake generator, go to the Visual Studio Installer and install the _C++ Clang Compiler for Windows_ and _MSBuild support for LLVM (clang-cl) toolset_ found under Individual components.

If you're using an alternate method, modify the CMake files as needed.

### TinyXML2
First, clone the repository into your chosen location. Navigate to the folder _libraries_ and create a new folder there named _tinyxml2_. Go into that folder and create two more folders, one called _include_ and one called _lib_. Go to the _include_ folder and create a new folder named _tinyxml2_.

Navigate to the TinyXML2 repository and copy the files _tinyxml2.cpp_ and _tinyxml2.h_. Navigate back to the model exporter repository and paste them into the _tinyxml2_ folder found in _include_ ([repository]/libraries/tinyxml2/include/tinyxml2).

Next, build the TinyXML2 lib file using CMake (there are plenty of tutorials online on how to use CMake if you are struggling). After you're done building, go to either the _Debug_ or _Release_ folder (depending on the one you built) and copy the file _tinyxml2.lib_.

Navigate back to the model exporter repository and paste the file into the _lib_ folder ([repository]/libraries/tinyxml2/lib). Rename it so either DEBUG or RELEASE is at the end of the filename, depending on whether you built debug or release (tinyxml2DEBUG.lib or tinyxml2RELEASE.lib).

If you have followed these steps correctly, TinyXML2 should now be added as an external library.

### libpng
This one's a bit of a ballache and for me the visual studio solutions provided in the source code were riddled with errors, so I instead rebuilt the solutions using CMakeLists, which I'll briefly go over how to do here.

Download libpng16 1.6.39 and unzip the folder to any location on your PC. Download zlib 1.2.11 and unzip the folder to any location on your PC.

Build zlib using CMake. Navigate to the build directory and open the solution file. Keep the build setting on _Debug_ for now. Right click _zlibstatic_ and then click _Build_ to build the file. Change the build setting to _Release_ and build the files again.

Next, copy both the compiled debug and release files. Create a new folder in the root of the libpng16 source code. You can name it whatever you want, I just called it zlib. Paste the files into the new folder.

Go back to the source code of zlib and copy the header files _zconf.h_ and _zlib.h_. Paste them into the new folder in the libpng16 source code.

Now you can build libpng using CMake, setting the _ZLIB_INCLUDE_DIR_ variable to the folder you created, setting _ZLIB_LIBRARY_DEBUG_ to _zlibstaticd.lib_, and setting _ZLIB_LIBRARY_RELEASE_ to _zlibstatic.lib_.

Copy _zlibstatic.lib_ and _zlibstaticd.lib_. Navigate to the build directory and paste the files there. Open the libpng solution file.

Choose either release or debug depending on which one you want to build for. Right click _png_ and click _Build_.

Create a new folder called _libpng_ in the _libraries_ folder in the model exporter's source code. Create two more folders inside the new folder called _include_ and _lib_. Create another new folder inside _include_ called _libpng_.

Copy _libpng16.dll_, _libpng16.lib_, _libpng16d.lib_, _zlibstatic.lib_, and _zlibstaticd.lib_ and paste them into the _lib_ folder.

Copy and paste _zconf.h_ and _zlib.h_ into _include/libpng_. Also copy and paste _png.h_, _pngconf.h_, and _pnglibconf.h_ into there (these files can be found in the root of the libpng source code).

If you have followed these steps correctly, libpng should now be added as an external library.

### Compile using Clang
If you are using Visual Studio as the CMake generator, build the Visual Studio solution using CMake. Open the generated solution file, go to Build -> Batch Build, and select whichever one you were working with, using _Gex2PS1ModelExporter_ and _Gex2PS1ModelNamesLister_ as the projects. Click build and the programs should be created.

You will also need the file _libpng16.dll_ in the same folder as the compiled program in order for it to run. Copy and paste it into whichever folder the program is in.

## Compatibility
Currently I have only tested the program on Windows 64-bit, and have only built a Windows release. However I am aiming to make the program cross-compatible with Linux.

## Credits
* Crystal Dynamics for their amazing game
* [TheSerioliOfNosgoth](https://github.com/TheSerioliOfNosgoth) for their work in reverse engineering Crystal Dynamics games
* [The Gex Discord](https://discord.gg/TeA7D4f) for their work in documenting and modding the Gex games
