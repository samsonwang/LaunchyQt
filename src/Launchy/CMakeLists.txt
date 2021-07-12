
cmake_minimum_required (VERSION 3.14.5)

project (Launchy)

get_filename_component (ROOT_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../ ABSOLUTE)
include (${ROOT_DIR}/CMakeModule/BaseConfig.cmake)
include (${ROOT_DIR}/CMakeModule/QtConfig.cmake)

include_directories (${ROOT_DIR})

list (APPEND TARGET_DEPEND_LIBRARIES
  LaunchyLib PluginPy Qt5::Widgets)

add_executable (${TARGET_NAME} ${SOURCES})
target_link_libraries (${TARGET_NAME} ${TARGET_DEPEND_LIBRARIES})
