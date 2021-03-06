
cmake_minimum_required (VERSION 3.14.5)

project ("Launchy")

set (CMAKE_C_STANDARD 11)
set (CMAKE_CXX_STANDARD 11)
set (CMAKE_CXX_STANDARD_REQUIRED ON)

set (CMAKE_BUILD_PARALLEL_LEVEL 4)

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

get_filename_component (SRC_DIR ${CMAKE_CURRENT_SOURCE_DIR}/ ABSOLUTE)
get_filename_component (DEPS_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../deps/ ABSOLUTE)
get_filename_component (TRANS_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../translations/ ABSOLUTE)

message (STATUS "SRC_DIR: ${SRC_DIR}")
message (STATUS "DEPS_DIR: ${DEPS_DIR}")
message (STATUS "TRANS_DIR: ${TRANS_DIR}")

include_directories (${SRC_DIR} ${DEPS_DIR})

##################################################
# Qt
# add CMAKE_PREFIX_PATH = <qt installed dir> in system PATH
# eg c:/Qt/Qt5.12.10/5.12.10/msvc2017
set (ENV{CMAKE_PREFIX_PATH} "c:/Qt/Qt5.12.10/5.12.10/msvc2017")
find_package (Qt5 COMPONENTS Widgets WinExtras Network LinguistTools REQUIRED)

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
set (ENV{Python_ROOT_DIR} "C:/Program Files (x86)/Python39-32")
find_package (Python COMPONENTS Interpreter Development REQUIRED)

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

set_target_properties(LaunchyLib PROPERTIES
  OUTPUT_NAME "Launchy")

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

add_custom_command (TARGET PluginPy POST_BUILD
  COMMAND ${CMAKE_COMMAND} -E copy_if_different
  $<TARGET_FILE:PluginPy> ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/$<CONFIG>/python/launchy.pyd)

# pluginpy util
add_custom_target (PluginPy.Util
  COMMAND ${CMAKE_COMMAND} -E copy_if_different
  ${SRC_DIR}/PluginPy/launchy_util.py ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/$<CONFIG>/python
  SOURCES ${SRC_DIR}/PluginPy/launchy_util.py)

##################################################
# Launchy main translation
file (GLOB TS_FILES_LAUNCHY
  ${TRANS_DIR}/*.ts)

qt5_create_translation (QM_FILES_LAUNCHY
  ${SRC_DIR}/Launchy
  ${SRC_DIR}/LaunchyLib
  ${SRC_DIR}/PluginPy
  ${TS_FILE_LAUNCHY})

set_source_files_properties(${TS_FILES_LAUNCHY} PROPERTIES
  OUTPUT_LOCATION ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/translations)
qt5_add_translation(QM_FILES_LAUNCHY ${TS_FILES_LAUNCHY})

add_custom_target (LaunchyQM
  DEPENDS ${QM_FILES_LAUNCHY})

add_custom_command (TARGET LaunchyQM POST_BUILD
  COMMAND ${CMAKE_COMMAND} -E copy_if_different
  ${QM_FILES_LAUNCHY} ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/$<CONFIG>/translations)

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

##################################################
# plugin: Runner
add_library (Plugin.Runner MODULE)

set_target_properties(Plugin.Runner PROPERTIES
  OUTPUT_NAME "Runner")

file (GLOB SOURCES_RUNNER
  ${SRC_DIR}/Plugins/Runner/*.h
  ${SRC_DIR}/Plugins/Runner/*.cpp)

# qt source files
file (GLOB SOURCES_QT_RUNNER
  ${SRC_DIR}/Plugins/Runner/*.ui
  ${SRC_DIR}/Plugins/Runner/*.qrc
  ${SRC_DIR}/Plugins/Runner/*.ts)
source_group ("Qt Files" FILES ${SOURCES_QT_RUNNER})
list (APPEND SOURCES_RUNNER ${SOURCES_QT_RUNNER})

target_sources (Plugin.Runner
  PRIVATE ${SOURCES_RUNNER})

target_link_libraries (Plugin.Runner
  PUBLIC Qt5::Core
  PRIVATE Qt5::Widgets
  PRIVATE LaunchyLib)

set_target_properties (Plugin.Runner PROPERTIES
  LIBRARY_OUTPUT_DIRECTORY_DEBUG "${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/Debug/plugins/Runner"
  LIBRARY_OUTPUT_DIRECTORY_RELEASE "${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/Release/plugins/Runner")

file (GLOB RESOURCES_RUNNER
  ${SRC_DIR}/Plugins/Runner/*.png)

add_custom_command (TARGET Plugin.Runner POST_BUILD
  COMMAND ${CMAKE_COMMAND} -E copy_if_different
  ${RESOURCES_RUNNER} ${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/$<CONFIG>/plugins/Runner)

##################################################
# plugin: Tasky
add_library (Plugin.Tasky MODULE)

set_target_properties(Plugin.Tasky PROPERTIES
  OUTPUT_NAME "Tasky")

file (GLOB SOURCES_TASKY
  ${SRC_DIR}/Plugins/Tasky/*.h
  ${SRC_DIR}/Plugins/Tasky/*.cpp)

# qt source files
file (GLOB SOURCES_QT_TASKY
  ${SRC_DIR}/Plugins/Tasky/*.ui
  ${SRC_DIR}/Plugins/Tasky/*.qrc
  ${SRC_DIR}/Plugins/Tasky/*.ts)
source_group ("Qt Files" FILES ${SOURCES_QT_TASKY})
list (APPEND SOURCES_TASKY ${SOURCES_QT_TASKY})

target_sources (Plugin.Tasky
  PRIVATE ${SOURCES_TASKY})

target_link_libraries (Plugin.Tasky
  PUBLIC Qt5::Core
  PRIVATE Qt5::Widgets Qt5::WinExtras
  PRIVATE LaunchyLib)

set_target_properties (Plugin.Tasky PROPERTIES
  LIBRARY_OUTPUT_DIRECTORY_DEBUG "${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/Debug/plugins/Tasky"
  LIBRARY_OUTPUT_DIRECTORY_RELEASE "${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/Release/plugins/Tasky")

file (GLOB RESOURCES_TASKY
  ${SRC_DIR}/Plugins/Tasky/*.png)

add_custom_command (TARGET Plugin.Tasky POST_BUILD
  COMMAND ${CMAKE_COMMAND} -E copy_if_different
  ${RESOURCES_TASKY} ${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/$<CONFIG>/plugins/Tasky)

##################################################
# plugin: UWPApp
add_library (Plugin.UWPApp MODULE)

set_target_properties(Plugin.UWPApp PROPERTIES
  OUTPUT_NAME "UWPApp")

file (GLOB SOURCES_UWPAPP
  ${SRC_DIR}/Plugins/UWPApp/*.h
  ${SRC_DIR}/Plugins/UWPApp/*.cpp)

# qt source files
file (GLOB SOURCES_QT_UWPAPP
  ${SRC_DIR}/Plugins/UWPApp/*.ui
  ${SRC_DIR}/Plugins/UWPApp/*.qrc
  ${SRC_DIR}/Plugins/UWPApp/*.ts)
source_group ("Qt Files" FILES ${SOURCES_QT_UWPAPP})
list (APPEND SOURCES_UWPAPP ${SOURCES_QT_UWPAPP})

target_sources (Plugin.UWPApp
  PRIVATE ${SOURCES_UWPAPP})

target_link_libraries (Plugin.UWPApp
  PUBLIC Qt5::Core
  PRIVATE Qt5::Widgets
  PRIVATE LaunchyLib
  PRIVATE user32 shell32 propsys)

set_target_properties (Plugin.UWPApp PROPERTIES
  LIBRARY_OUTPUT_DIRECTORY_DEBUG "${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/Debug/plugins/UWPApp"
  LIBRARY_OUTPUT_DIRECTORY_RELEASE "${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/Release/plugins/UWPApp")

##################################################
# plugin: Verby
add_library (Plugin.Verby MODULE)

set_target_properties(Plugin.Verby PROPERTIES
  OUTPUT_NAME "Verby")

file (GLOB SOURCES_VERBY
  ${SRC_DIR}/Plugins/Verby/*.h
  ${SRC_DIR}/Plugins/Verby/*.cpp)

# qt source files
file (GLOB SOURCES_QT_VERBY
  ${SRC_DIR}/Plugins/Verby/*.ui
  ${SRC_DIR}/Plugins/Verby/*.qrc
  ${SRC_DIR}/Plugins/Verby/*.ts)
source_group ("Qt Files" FILES ${SOURCES_QT_VERBY})
list (APPEND SOURCES_VERBY ${SOURCES_QT_VERBY})

target_sources (Plugin.Verby
  PRIVATE ${SOURCES_VERBY})

target_link_libraries (Plugin.Verby
  PUBLIC Qt5::Core
  PRIVATE Qt5::Widgets
  PRIVATE LaunchyLib)

set_target_properties (Plugin.Verby PROPERTIES
  LIBRARY_OUTPUT_DIRECTORY_DEBUG "${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/Debug/plugins/Verby"
  LIBRARY_OUTPUT_DIRECTORY_RELEASE "${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/Release/plugins/Verby")

file (GLOB RESOURCES_VERBY
  ${SRC_DIR}/Plugins/Verby/*.png)

add_custom_command (TARGET Plugin.Verby POST_BUILD
  COMMAND ${CMAKE_COMMAND} -E copy_if_different
  ${RESOURCES_VERBY} ${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/$<CONFIG>/plugins/Verby)

##################################################
# plugin: CalcPy
add_custom_target (PluginPy.CalcPy
  COMMAND ${CMAKE_COMMAND} -E copy_directory
  ${SRC_DIR}/Plugins/CalcyPy ${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/$<CONFIG>/plugins/CalcyPy)

##################################################
# plugin: PydiryPy
add_custom_target (PluginPy.PydiryPy
  COMMAND ${CMAKE_COMMAND} -E copy_directory
  ${SRC_DIR}/Plugins/PydiryPy ${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/$<CONFIG>/plugins/PydiryPy)

##################################################
# plugin: WebSearchPy
add_custom_target (PluginPy.WebSearchPy
  COMMAND ${CMAKE_COMMAND} -E copy_directory
  ${SRC_DIR}/Plugins/WebSearchPy ${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/$<CONFIG>/plugins/WebSearchPy)
