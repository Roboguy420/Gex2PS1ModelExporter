# CMakeList.txt : CMake project for Gex2PS1ModelExporter, include source and define
# project specific logic here.
#
cmake_minimum_required (VERSION 3.8)

set(CMAKE_GENERATOR_TOOLSET "ClangCL")

add_compile_definitions($<$<CONFIG:Release>:_ITERATOR_DEBUG_LEVEL=0>)

# Enable Hot Reload for MSVC compilers if supported.
if (POLICY CMP0141)
  cmake_policy(SET CMP0141 NEW)
  set(CMAKE_MSVC_DEBUG_INFORMATION_FORMAT "$<IF:$<AND:$<C_COMPILER_ID:MSVC>,$<CXX_COMPILER_ID:MSVC>>,$<$<CONFIG:Debug,RelWithDebInfo>:EditAndContinue>,$<$<CONFIG:Debug,RelWithDebInfo>:ProgramDatabase>>")
endif()

project ("Gex2PS1ModelExporter")

# Add source to this project's executable.
add_executable (Gex2PS1ModelExporter "Gex2PS1ModelExporter.cpp" "Gex2PS1TextureExporter.cpp")

if (CMAKE_VERSION VERSION_GREATER 3.12)
  set_property(TARGET Gex2PS1ModelExporter PROPERTY CXX_STANDARD 20)
endif()

# TODO: Add tests and install targets if needed.

target_include_directories(Gex2PS1ModelExporter PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/include")

target_link_libraries(Gex2PS1ModelExporter PUBLIC 
	debug "${CMAKE_CURRENT_SOURCE_DIR}/libraries/tinyxml2/lib/tinyxml2d.lib"
	optimized "${CMAKE_CURRENT_SOURCE_DIR}/libraries/tinyxml2/lib/tinyxml2.lib")

target_include_directories(Gex2PS1ModelExporter PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/libraries/tinyxml2/include")

target_link_libraries(Gex2PS1ModelExporter PUBLIC
	debug "${CMAKE_CURRENT_SOURCE_DIR}/libraries/libpng/lib/libpng16d.lib"
	optimized "${CMAKE_CURRENT_SOURCE_DIR}/libraries/libpng/lib/libpng16.lib")

target_include_directories(Gex2PS1ModelExporter PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/libraries/libpng/include")



add_executable (Gex2PS1ModelNamesLister "Gex2PS1ModelNamesLister.cpp")

if (CMAKE_VERSION VERSION_GREATER 3.12)
  set_property(TARGET Gex2PS1ModelNamesLister PROPERTY CXX_STANDARD 20)
endif()

target_include_directories(Gex2PS1ModelNamesLister PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/include")



add_executable(TextureDebug ".debugTextureDebug.cpp")

if (CMAKE_VERSION VERSION_GREATER 3.12)
  set_property(TARGET TextureDebug PROPERTY CXX_STANDARD 20)
endif()

target_include_directories(TextureDebug PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/include")

target_link_libraries(TextureDebug PUBLIC
	debug "${CMAKE_CURRENT_SOURCE_DIR}/libraries/libpng/lib/libpng16d.lib"
	optimized "${CMAKE_CURRENT_SOURCE_DIR}/libraries/libpng/lib/libpng16.lib")

target_include_directories(TextureDebug PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/libraries/libpng/include")