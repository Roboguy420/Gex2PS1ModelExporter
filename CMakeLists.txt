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

set(SOURCE_FILES_EXPORTER
  ${CMAKE_CURRENT_SOURCE_DIR}/src/SharedFunctions.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/src/ModelExporter.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/src/TextureExporter.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/src/ModelNamesLister.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/src/XMLExport.cpp
)

set(HEADER_FILES_EXPORTER
  ${CMAKE_CURRENT_SOURCE_DIR}/include/SharedFunctions.h
  ${CMAKE_CURRENT_SOURCE_DIR}/include/ModelExporter.h
  ${CMAKE_CURRENT_SOURCE_DIR}/include/TextureExporter.h
  ${CMAKE_CURRENT_SOURCE_DIR}/include/ModelNamesLister.h
  ${CMAKE_CURRENT_SOURCE_DIR}/include/ModelStructs.h
  ${CMAKE_CURRENT_SOURCE_DIR}/include/TextureStructs.h
  ${CMAKE_CURRENT_SOURCE_DIR}/include/Constants.h
  ${CMAKE_CURRENT_SOURCE_DIR}/include/XMLExport.h
)

if (WIN32)
  list(APPEND SOURCE_FILES_EXPORTER
    ${CMAKE_CURRENT_SOURCE_DIR}/lib/getopt/include/getopt.c
  )
  list(APPEND HEADER_FILES_EXPORTER
    ${CMAKE_CURRENT_SOURCE_DIR}/lib/getopt/include/getopt.h
  )
endif()

add_executable (gex2ps1modelexporter ${SOURCE_FILES_EXPORTER} ${HEADER_FILES_EXPORTER})

set_property(TARGET gex2ps1modelexporter PROPERTY CXX_STANDARD 20)

# TODO: Add tests and install targets if needed.

target_include_directories(gex2ps1modelexporter PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/include")

if (WIN32)
  target_include_directories(gex2ps1modelexporter PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/lib/getopt/include")
endif()


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

  target_include_directories(gex2ps1modelexporter PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/lib/libpng/include")
  target_include_directories(gex2ps1modelexporter PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/lib/tinyxml2/include")

  target_link_libraries(gex2ps1modelexporter
  debug "${CMAKE_CURRENT_SOURCE_DIR}/lib/libpng/lib/libpng16d.lib"
  optimized "${CMAKE_CURRENT_SOURCE_DIR}/lib/libpng/lib/libpng16.lib")
  target_link_libraries(gex2ps1modelexporter
  debug "${CMAKE_CURRENT_SOURCE_DIR}/lib/tinyxml2/lib/tinyxml2d.lib"
  optimized "${CMAKE_CURRENT_SOURCE_DIR}/lib/tinyxml2/lib/tinyxml2.lib")

endif()



install(TARGETS gex2ps1modelexporter DESTINATION bin)
