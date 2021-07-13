
cmake_minimum_required (VERSION 3.14.5)

project (Launchy)

get_filename_component (SRC_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../ ABSOLUTE)
get_filename_component (DEPS_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../../deps/ ABSOLUTE)

include (${SRC_DIR}/CMakeModule/BaseConfig.cmake)
include (${SRC_DIR}/CMakeModule/QtConfig.cmake)

include_directories (${SRC_DIR})
include_directories (${DEPS_DIR})

file (GLOB SOURCES_SINGLEAPP
  ${DEPS_DIR}/SingleApplication/*.h
  ${DEPS_DIR}/SingleApplication/*.cpp)
list (APPEND SOURCES ${SOURCES_SINGLEAPP})
source_group ("_SingleApp Files" FILES ${SOURCES_SINGLEAPP})

file (GLOB SOURCES_HOTKEY
  ${DEPS_DIR}/QHotKey/*.h
  ${DEPS_DIR}/QHotKey/QHotKey.cpp
  ${DEPS_DIR}/QHotKey/QHotKeyP.cpp
  ${DEPS_DIR}/QHotKey/QHotKeyWin.cpp)
list (APPEND SOURCES ${SOURCES_HOTKEY})
source_group ("_HotKey Files" FILES ${SOURCES_HOTKEY})

add_compile_definitions (QAPPLICATION_CLASS=QApplication)

list (APPEND TARGET_DEPEND_LIBRARIES
  LaunchyLib PluginPy
  userenv netapi32
  Qt5::Widgets Qt5::WinExtras Qt5::Network)

add_executable (${TARGET_NAME} ${SOURCES})
target_link_libraries (${TARGET_NAME} ${TARGET_DEPEND_LIBRARIES})
