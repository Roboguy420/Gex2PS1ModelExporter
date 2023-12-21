# Gex 2 PS1 Model Exporter
This is a simple command line program written in C++ that exports Gex 2 PS1 model files to COLLADA .dae model files. This is my first ever C++ program so it's a little rough around the edges and has some novice bits of code, but at the end of the day it works and I'm glad it does.

## Download
Binaries can be found in the [releases](https://github.com/Roboguy420/Gex2PS1ModelExporter/releases). Download the latest release for your OS to get all the latest features.

Windows 64-bit release is Gex2PS1ModelExporter-windows-64bit.zip

Arch 64-bit release is gex2ps1modelexporter-[ver]-[rel]-x86_64.pkg.tar.zst

RPM 64-bit release is gex2ps1modelexporter-[ver]-[rel].x86_64.rpm

Debian AMD64 release is gex2ps1modelexporter_[ver]-[rel]_amd64.deb

## Usage
There is 1 needed parameter in the program. This is the **input file**, the model file from Gex 2 (extension is _.drm_). The parameter can be either the local location of the file, relative to the current working directory, or the exact location (specified on most OS's as having a forward slash at the start. On windows you put the volume at the start, e.g. C:\\)

There are 3 additional flags, 2 of them with arguments and 1 of them is non-argument.

The 1st additional flag is the **output folder**, specified by _-o_ or _--out_. This is the folder where the models will be output. Note that this does not create a folder with the name of the parameter; the folder must be preexisting in order to work. If this flag does not exist, it uses the current working directory.

The 2nd additional flag is the **model index**, specified by _-i_ or _--index_. This is the index of whichever model you want to export. The value -1 is reserved for exporting everything. The value 0 is reserved for the level geometry. Any value >0 is used for the several other models stored in the input file. If this flag does not exist, it defaults to exporting everything.

The 3rd additional flag is the **names lister flag**, specified by _-l_ or _--list_. This is a non-argument flag that simply tells the program to list the names of all the models found within the file along with their respective index to stdout, rather than exporting them to .dae files. Note that you can include the output and index flags alongside the list flag without errors occurring, but the program won't do anything with the information aside from the preexisting error and existence checks.

Usage on the command line is as follows:
```
> gex2ps1modelexporter file [-o --out folder] [-i --index number] [-l --list]
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
* **2:** Improperly formatted arguments
* **3:** Input file not found
* **4:** Failed to read input file
* **5:** Output folder in 2nd argument does not exist
* **6:** 3rd argument is not a number -1 or higher
* **7:** End of stream exception (usually the result of a corrupt or unusually formatted DRM file)
* **8:** At least 1 texture failed to export
* **9:** At least 1 successful export, others had failures
* **10:** No successful exports, all attempts failed

## Compilation & Building - Windows
This program requires:
* [TinyXML2](https://github.com/leethomason/tinyxml2) to make the DAE files
* [libpng](https://sourceforge.net/projects/libpng/files/) to export textures to PNG
* [zlib](https://sourceforge.net/projects/libpng/files/zlib/) to compile libpng
* [getopt](https://github.com/mirror/mingw-w64/tree/master) for the POSIX-style argument parser
* [CMake](https://cmake.org) to build

Install CMake if you haven't already and clone the TinyXML2 repository onto your computer.

If you're sticking with using Visual Studio as the CMake generator, go to the Visual Studio Installer and install the _C++ Clang Compiler for Windows_ and _MSBuild support for LLVM (clang-cl) toolset_ found under Individual components.

If you're using an alternate method, modify the CMake files as needed.

### TinyXML2 (static library)
First, clone the repository into your chosen location. Navigate to the folder _lib_ and create a new folder there named _tinyxml2_. Go into that folder and create two more folders, one called _include_ and one called _lib_.

Navigate to the TinyXML2 repository and copy the files _tinyxml2.cpp_ and _tinyxml2.h_. Navigate back to the model exporter repository and paste them into the _include_ folder ([repository]/lib/tinyxml2/include).

Next, build the TinyXML2 lib file using CMake (there are plenty of tutorials online on how to use CMake if you are struggling). After you're done building, go to either the _Debug_ or _Release_ folder (depending on the one you built) and copy the file _tinyxml2.lib_.

Navigate back to the model exporter repository and paste the file into the _lib_ folder ([repository]/lib/tinyxml2/lib). If it is a debug build, add 'd' to the end of the filename (tinyxml2d.lib).

If you have followed these steps correctly, TinyXML2 should now be added as an external library.

### libpng (static library)
This one's a bit of a ballache and for me the visual studio solutions provided in the source code were riddled with errors, so I instead rebuilt the solutions using CMakeLists, which I'll briefly go over how to do here.

Download libpng16 1.6.39 and unzip the folder to any location on your PC. Download zlib 1.2.11 and unzip the folder to any location on your PC.

Build zlib using CMake. Navigate to the build directory and open the solution file. Keep the build setting on _Debug_ for now. Right click _zlibstatic_ and then click _Build_ to build the file. Change the build setting to _Release_ and build the files again.

Next, copy both the compiled debug and release files. Create a new folder in the root of the libpng16 source code. You can name it whatever you want, I just called it zlib. Paste the files into the new folder.

Go back to the source code of zlib and copy the header files _zconf.h_ and _zlib.h_. Paste them into the new folder in the libpng16 source code.

Now you can build libpng using CMake, setting the _ZLIB_INCLUDE_DIR_ variable to the folder you created, setting _ZLIB_LIBRARY_DEBUG_ to _zlibstaticd.lib_, and setting _ZLIB_LIBRARY_RELEASE_ to _zlibstatic.lib_.

Copy _zlibstatic.lib_ and _zlibstaticd.lib_. Navigate to the build directory and paste the files there. Open the libpng solution file.

Choose either release or debug depending on which one you want to build for. Right click _png_ and click _Build_.

Create a new folder called _libpng_ in the _lib_ folder in the model exporter's source code. Create two more folders inside the new folder called _include_ and _lib_.

Copy _libpng16.dll_, _libpng16.lib_, _libpng16d.lib_, _zlibstatic.lib_, and _zlibstaticd.lib_ and paste them into the _lib_ folder.

Copy and paste _zconf.h_ and _zlib.h_ into _include_. Also copy and paste _png.h_, _pngconf.h_, and _pnglibconf.h_ into there (these files can be found in the root of the libpng source code).

If you have followed these steps correctly, libpng should now be added as an external library.

### getopt (from source)
I didn't even bother compiling this one into a library, since it's much easier to just include the header file and compile the source file in the main binary and they aren't even that big (their combined filesize is about 20kb). This is the easiest one to do.

Go to the Github repository for mingw-w64 that I linked. Navigate to _mingw-w64-crt/misc_ and download the source file _getopt.c_. Next, navigate to _mingw-w64-headers/crt_ and download the header file _getopt.h_.

Create a new folder in the _lib_ folder of the model exporter and name it _getopt_. Create a folder inside of that and call it _include_. Copy and paste _getopt.c_ and _getopt.h_ into the newly created _include_ folder.

If all has worked well, _getopt_ and all its functions should now be findable and usable with CMake and within your IDE/code editor.

The Linux build does not require this step as Linux is more POSIX-compliant than Windows, and the _getopt_ functions are a built-in part of its standard C implementation.

### Compile
If you are using Visual Studio as the CMake generator, build the Visual Studio solution using CMake. Open the generated solution file, go to Build -> Batch Build, and select whichever one you were working with, using _Gex2PS1ModelExporter_ as the project. Click build and the programs should be created.

You will also need the file _libpng16.dll_ in the same folder as the compiled program in order for it to run. Copy and paste it into whichever folder the program is in.

## Compilation & Building - Linux
This process assumes you have a Linux distribution installed and are familiar with its package manager. If you are uncomfortable with your system's package manager and instead use something like Snap or Flatpak, I cannot help you here.

Generally you can just follow the logic of the package buildfiles provided in the build-pkgs directory of the source code, but if you are still stuck trying to locally build the program, this section can help.

### Install the Needed Packages
This program requires CMake to build. Install the cmake package on your distribution with its package manager.

To retrieve the source code, you will need git. Install the git package.

You will also need to install libpng and tinyxml2. These may be named differently depending on your distribution. You may also need to install the developer packages for the build process, depending on your distribution.

Lastly, you will need a compiler and a standard C library. The ones I chose and are confirmed to work are the GNU Compiler Collection (gcc and g++) and the GNU C Library (glibc). Install these.

### Build
Clone the repository into a folder of your choice (e.g. /home/[user]/src).

Navigate to the root of the repository folder and run the following command:
```
> cmake --preset [preset]
```

The available presets are _x64-release-linux_ and _x64-debug-linux_. These presets use the compilers, generators, and etc that I use when building for Linux (you can find their exact details in CMakePresets.json). If you would like to change some of the attributes, add more flags to the cmake command as necessary.

You may also want to include a `-DCMAKE_INSTALL_PREFIX=[install prefix]` flag, if you wish to install the program in a location other than /usr/local. (By default CMake installs it into /usr/local, this is the conventional location on Linux for programs that are not managed by any package manager)

Change directory to where it was built:
```
> cd out/build/[preset]
```

...and compile the programs:
```
> cmake --build .
```

If all has went well, gex2ps1modelexporter will be built in the folder with no errors.

### Install
If you wish to install the program, navigate to where the makefile is (this should be out/build/[preset] relative to the source code).

Run the following command:
```
> cmake --install .
```

If all has went well, gex2ps1modelexporter will be installed in the chosen installation folder with no errors.

## Compatibility
The source code aims to be as OS and compiler agnostic as possible. The official releases are for 64-bit Windows, x86-64 Arch Linux, x86-64 Red Hat-based Linux, and AMD64 Debian GNU/Linux. They have been tested and found to be working on Windows 10 64-bit, Artix Linux, Fedora Linux x86-64, and Debian Sid (unstable Trixie as of writing) AMD64 respectively.

## Additional Notes
These are some other things that are important to keep in mind when using the program.
* Due to its small scale and lack of wide usage, the program is relatively untested. Use caution when selecting the import file. Only try to export models from files that you know are from a properly dumped version of the game.
* During reading of an imported .drm file, the program will create a temporary file named "Gex2PS1ModelExporterTempfile.drm" in either the temp directory, or if its environment variable doesn't exist, the current working directory. If the program halts prematurely, this file may still persist in that directory, so make sure to keep a note of it and delete it if it still exists after the program has closed.

## Credits
* Crystal Dynamics for their amazing game
* [TheSerioliOfNosgoth](https://github.com/TheSerioliOfNosgoth) for their work in reverse engineering Crystal Dynamics games
* [The Gex Discord](https://discord.gg/TeA7D4f) for their work in documenting and modding the Gex games
