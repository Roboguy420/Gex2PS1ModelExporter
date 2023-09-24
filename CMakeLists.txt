# CMakeList.txt : CMake project for Gex2PS1ModelExporter, include source and define
# project specific logic here.
#
cmake_minimum_required (VERSION 3.8)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

add_compile_definitions($<$<CONFIG:Release>:_ITERATOR_DEBUG_LEVEL=0>)

# Enable Hot Reload for MSVC compilers if supported.
if (POLICY CMP0141)
  cmake_policy(SET CMP0141 NEW)
  set(CMAKE_MSVC_DEBUG_INFORMATION_FORMAT "$<IF:$<AND:$<C_COMPILER_ID:MSVC>,$<CXX_COMPILER_ID:MSVC>>,$<$<CONFIG:Debug,RelWithDebInfo>:EditAndContinue>,$<$<CONFIG:Debug,RelWithDebInfo>:ProgramDatabase>>")
endif()

project ("Gex2PS1ModelExporter")


# Model exporter
file(GLOB HEADER_FILES_EXPORTER include/*.h)
add_executable (gex2ps1modelexporter "Gex2PS1ModelExporter.cpp" "Gex2PS1TextureExporter.cpp" ${HEADER_FILES_EXPORTER})

set_property(TARGET gex2ps1modelexporter PROPERTY CXX_STANDARD 20)

# TODO: Add tests and install targets if needed.

target_include_directories(gex2ps1modelexporter PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/include")


# Linking libraries, choose shared or static when running cmake

if (USE_SHARED_LIBRARIES)
  set(tinyxml2_PATHS
    /usr
  )

  find_package(PNG REQUIRED)
  find_package(tinyxml2 REQUIRED)

  find_library(tinyxml2_LIBRARIES NAMES tinyxml2 PATH_SUFFIXES lib PATHS ${tinyxml2_PATHS})
  find_path(tinyxml2_INCLUDE_DIRS tinyxml2.h PATH_SUFFIXES include PATHS ${tinyxml2_PATHS})

  target_include_directories(gex2ps1modelexporter PRIVATE "${PNG_INCLUDE_DIRS}")
  target_include_directories(gex2ps1modelexporter PRIVATE "${tinyxml2_INCLUDE_DIRS}")

  target_link_libraries(gex2ps1modelexporter "${PNG_LIBRARIES}")
  target_link_libraries(gex2ps1modelexporter "${tinyxml2_LIBRARIES}")

else()

  target_include_directories(gex2ps1modelexporter PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}\\libraries\\libpng\\include")
  target_include_directories(gex2ps1modelexporter PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}\\libraries\\tinyxml2\\include")

  target_link_libraries(gex2ps1modelexporter
  debug "${CMAKE_CURRENT_SOURCE_DIR}\\libraries\\libpng\\lib\\libpng16d.lib"
  optimized "${CMAKE_CURRENT_SOURCE_DIR}\\libraries\\libpng\\lib\\libpng16.lib")
  target_link_libraries(gex2ps1modelexporter
  debug "${CMAKE_CURRENT_SOURCE_DIR}\\libraries\\tinyxml2\\lib\\tinyxml2d.lib"
  optimized "${CMAKE_CURRENT_SOURCE_DIR}\\libraries\\tinyxml2\\lib\\tinyxml2.lib")

endif()


# Model names lister
set(HEADER_FILES_NAMES_LISTER include/Gex2PS1SharedFunctions.h)
add_executable (gex2ps1modelnameslister "Gex2PS1ModelNamesLister.cpp" ${HEADER_FILES_NAMES_LISTER})

set_property(TARGET gex2ps1modelnameslister PROPERTY CXX_STANDARD 20)

target_include_directories(gex2ps1modelnameslister PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/include")

install(TARGETS gex2ps1modelexporter DESTINATION bin)
install(TARGETS gex2ps1modelnameslister DESTINATION bin)
