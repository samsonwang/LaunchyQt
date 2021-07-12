
cmake_minimum_required (VERSION 3.14.5)

project (LaunchyLib)

get_filename_component (ROOT_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../ ABSOLUTE)
include (${ROOT_DIR}/CMakeModule/BaseConfig.cmake)
include (${ROOT_DIR}/CMakeModule/QtConfig.cmake)

include_directories (${ROOT_DIR})
add_compile_definitions (LAUNCHY_LIB)

list (APPEND TARGET_DEPEND_LIBRARIES
  Qt5::Widgets)

add_library (${TARGET_NAME} ${SOURCES})
target_link_libraries (${TARGET_NAME} ${TARGET_DEPEND_LIBRARIES})
