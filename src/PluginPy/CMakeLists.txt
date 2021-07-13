
cmake_minimum_required (VERSION 3.14.5)

project (PluginPy)

get_filename_component (ROOT_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../ ABSOLUTE)
get_filename_component (DEPS_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../../deps/ ABSOLUTE)

include (${ROOT_DIR}/CMakeModule/BaseConfig.cmake)
include (${ROOT_DIR}/CMakeModule/QtConfig.cmake)
include (${ROOT_DIR}/CMakeModule/PythonConfig.cmake)

include_directories (${ROOT_DIR})
include_directories (${DEPS_DIR})
include_directories (${Python_INCLUDE_DIRS})

add_compile_definitions (PLUGINPY_LIB)

list (APPEND TARGET_DEPEND_LIBRARIES
  LaunchyLib Qt5::Widgets ${Python_LIBRARIES})

add_library (${TARGET_NAME} ${SOURCES})
target_link_libraries (${TARGET_NAME} ${TARGET_DEPEND_LIBRARIES})
