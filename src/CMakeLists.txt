
cmake_minimum_required (VERSION 3.14.5)

project ("Launchy")

set (CMAKE_C_STANDARD 11)
set (CMAKE_CXX_STANDARD 11)
set (CMAKE_CXX_STANDARD_REQUIRED ON)

set (CMAKE_BUILD_PARALLEL_LEVEL 4)

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

get_filename_component (SRC_DIR ${CMAKE_CURRENT_SOURCE_DIR}/ ABSOLUTE)
get_filename_component (DEPS_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../deps/ ABSOLUTE)

include_directories (${SRC_DIR} ${DEPS_DIR})

message (STATUS "SRC_DIR: ${SRC_DIR}")
message (STATUS "DEPS_DIR: ${DEPS_DIR}")

##################################################
# Qt
# add CMAKE_PREFIX_PATH = <qt installed dir> in system PATH
find_package (Qt5 COMPONENTS Widgets WinExtras Network REQUIRED)

set (CMAKE_AUTOUIC ON)
set (CMAKE_AUTOMOC ON)
set (CMAKE_AUTORCC ON)

# source group for qt moc files
set_property (GLOBAL PROPERTY AUTOUIC_SOURCE_GROUP "Generated Files")
set_property (GLOBAL PROPERTY AUTOMOC_SOURCE_GROUP "Generated Files")
set_property (GLOBAL PROPERTY AUTORCC_SOURCE_GROUP "Generated Files")

# link qtmain.lib
if (POLICY CMP0020)
  cmake_policy (SET CMP0020 NEW)
endif ()

# hide console in release
if (WIN32)
  set (CMAKE_WIN32_EXECUTABLE TRUE)
endif ()

##################################################
# Python
find_package (Python COMPONENTS Interpreter Development)

message ("Python_FOUND: ${Python_FOUND}")
message ("Python_VERSION: ${Python_VERSION}")
message ("Python_Development_FOUND: ${Python_Development_FOUND}")
message ("Python_LIBRARIES: ${Python_LIBRARIES}")

##################################################
# Platform
if (MSVC)
  add_compile_options (/W3 /utf-8)
  # add_compile_options (/wd4819) # disable warning C4819: unicode charset
  # add_compile_options (/MP) # multi-processor compilation

  add_compile_definitions (_CRT_SECURE_NO_WARNINGS)
  add_compile_definitions (NOMINMAX)
  add_compile_definitions (WIN32_LEAN_AND_MEAN)

  # set (CMAKE_CONFIGURATION_TYPES "Debug;Release")

#  set (CMAKE_EXE_LINKER_FLAGS_DEBUG "/SUBSYSTEM:CONSOLE")
#  set (CMAKE_EXE_LINKER_FLAGS_RELEASE "/SUBSYSTEM:WINDOWS")

  set (CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
  set (CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

else ()
  add_compile_options (-Wall -Wextra)
  set (CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -g")

  set (CMAKE_LIBRARY_OUTPUT_DIRECTORY ~/Launchy)
  set (CMAKE_RUNTIME_OUTPUT_DIRECTORY ~/Launchy)
endif ()


##################################################
# Launchy
add_executable (Launchy)

# source files
file (GLOB SOURCES_LAUNCHY
  ${SRC_DIR}/Launchy/*.h
  ${SRC_DIR}/Launchy/*.cpp)

# sources files for operation system
if (WIN32)
  file (GLOB SOURCES_PLATFORM_LAUNCHY
    ${SRC_DIR}/Launchy/Windows/*.h
    ${SRC_DIR}/Launchy/Windows/*.cpp
    ${SRC_DIR}/Launchy/Windows/launchy.rc)
  source_group ("Resource Files" FILES ${SRC_DIR}/Launchy/Windows/launchy.rc)
  target_include_directories (Launchy PRIVATE ${SRC_DIR}/Launchy/Windows)
else ()
  file (GLOB SOURCES_PLATFORM_LAUNCHY
    ${SRC_DIR}/Launchy/Linux/*.h
    ${SRC_DIR}/Launchy/Linux/*.cpp)
  target_include_directories (Launchy PRIVATE ${SRC_DIR}/Launchy/Linux)
endif (WIN32)
list (APPEND SOURCES_LAUNCHY ${SOURCES_PLATFORM_LAUNCHY})
target_include_directories (Launchy PRIVATE ${SRC_DIR}/Launchy)

# qt source files
file (GLOB SOURCES_QT_LAUNCHY
  ${SRC_DIR}/Launchy/*.ui
  ${SRC_DIR}/Launchy/*.qrc
  ${SRC_DIR}/Launchy/*.ts)
source_group ("Qt Files" FILES ${SOURCES_QT_LAUNCHY})
list (APPEND SOURCES_LAUNCHY ${SOURCES_QT_LAUNCHY})


target_sources (Launchy
  PRIVATE ${SOURCES_LAUNCHY}
  PRIVATE ${SOURCES_SINGLEAPP}
  PRIVATE ${SOURCES_HOTKEY})

target_link_libraries (Launchy
  LaunchyLib PluginPy
  QSingleApplication QHotKey
  userenv netapi32
  Qt5::Widgets Qt5::WinExtras Qt5::Network)

##################################################
# LaunchyLib
# source files
add_library (LaunchyLib SHARED)

file (GLOB SOURCES_LAUNCHYLIB
  ${SRC_DIR}/LaunchyLib/*.h
  ${SRC_DIR}/LaunchyLib/*.cpp)

target_sources (LaunchyLib
  PRIVATE ${SOURCES_LAUNCHYLIB})

target_link_libraries (LaunchyLib
  PRIVATE Qt5::Core Qt5::Widgets)

##################################################
# PluginPy
add_library (PluginPy SHARED)

file (GLOB SOURCES_PLUGINPY
  ${SRC_DIR}/PluginPy/*.h
  ${SRC_DIR}/PluginPy/*.cpp)

target_sources (PluginPy
  PRIVATE ${SOURCES_PLUGINPY})

target_include_directories (PluginPy
  PRIVATE ${Python_INCLUDE_DIRS})

target_link_libraries (PluginPy
  PUBLIC LaunchyLib
  PRIVATE Qt5::Core Qt5::Widgets
  PRIVATE ${Python_LIBRARIES})

##################################################
# QSingleApplication
add_library (QSingleApplication STATIC)

file (GLOB SOURCES_SINGLEAPP
  ${DEPS_DIR}/SingleApplication/*.h
  ${DEPS_DIR}/SingleApplication/*.cpp)

target_compile_definitions (QSingleApplication PUBLIC QAPPLICATION_CLASS=QApplication)

target_sources (QSingleApplication
  PRIVATE ${SOURCES_SINGLEAPP})

target_link_libraries (QSingleApplication
  PUBLIC Qt5::Core Qt5::Widgets Qt5::Network)

##################################################
# QHotKey
add_library (QHotKey STATIC)

file (GLOB SOURCES_HOTKEY
  ${DEPS_DIR}/QHotKey/*.h
  ${DEPS_DIR}/QHotKey/QHotKey.cpp
  ${DEPS_DIR}/QHotKey/QHotKeyP.cpp
  ${DEPS_DIR}/QHotKey/QHotKeyWin.cpp)

target_sources (QHotKey
  PRIVATE ${SOURCES_HOTKEY})

target_link_libraries (QHotKey
  PUBLIC Qt5::Core
  PRIVATE Qt5::Widgets)
