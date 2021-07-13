
message (STATUS "Setting up project: ${PROJECT_NAME}")

set (TARGET_NAME ${PROJECT_NAME})
set (TARGET_DEPEND_LIBRARIES "")

# source files
file (GLOB SOURCES
  *.h
  *.hpp
  *.c
  *.cpp)

# sources files for operation system
if (WIN32)
  file (GLOB SOURCES_PLATFORM
    Windows/*.h
    Windows/*.cpp)
  include_directories (Windows)
else ()
  file (GLOB SOURCES_PLATFORM
    Linux/*.h
    Linux/*.cpp)
  include_directories (Linux)
endif (WIN32)

list (APPEND SOURCES ${SOURCES_PLATFORM})

# json files
file (GLOB SOURCES_JSON
  *.json)
list (APPEND SOURCES ${SOURCES_JSON})
source_group ("Json Files" FILES ${SOURCES_JSON})

# include parent directory
get_filename_component (PUBLIC_PARENT_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../ ABSOLUTE)
include_directories (${PUBLIC_PARENT_DIR} ${CMAKE_CURRENT_SOURCE_DIR})
# message (STATUS "BINARY_DIR: ${CMAKE_BINARY_DIR}")
# message (STATUS "CURRENT_BINARY_DIR: ${CMAKE_CURRENT_BINARY_DIR}")
# message (STATUS "PUBLIC_PARENT_DIR: ${PUBLIC_PARENT_DIR}")

set (CMAKE_C_STANDARD 11)
set (CMAKE_CXX_STANDARD 11)
set (CMAKE_CXX_STANDARD_REQUIRED ON)

set (CMAKE_BUILD_PARALLEL_LEVEL 4)

if (NOT DEFINED BUILD_SHARED_LIBS)
  set (BUILD_SHARED_LIBS TRUE)
endif ()

if (MSVC)
  add_compile_options (/W3 /utf-8)
#  add_compile_options (/wd4819) # disable warning C4819: unicode charset
#  add_compile_options (/MP) # multi-processor compilation

  add_compile_definitions (_CRT_SECURE_NO_WARNINGS)
  add_compile_definitions (NOMINMAX)
  add_compile_definitions (WIN32_LEAN_AND_MEAN)

  #set (CMAKE_CONFIGURATION_TYPES "Debug;Release")
else ()
  add_compile_options (-Wall -Wextra)
  set (CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -g")
endif ()

# auto push and pop cmake policy
if (POLICY CMP0011)
  cmake_policy (SET CMP0011 NEW)
endif ()

# find package by <PackageName>_ROOT
if (POLICY CMP0074)
  cmake_policy (SET CMP0074 NEW)
endif ()

# link library relative path
if (POLICY CMP0015)
  cmake_policy (SET CMP0015 NEW)
endif ()
