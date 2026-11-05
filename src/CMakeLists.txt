
cmake_minimum_required (VERSION 3.14)

project ("Launchy")

set (CMAKE_C_STANDARD 11)
set (CMAKE_CXX_STANDARD 11)
set (CMAKE_CXX_STANDARD_REQUIRED ON)

set (CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

get_filename_component (SRC_DIR ${CMAKE_CURRENT_SOURCE_DIR}/ ABSOLUTE)
get_filename_component (DEPS_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../deps/ ABSOLUTE)
get_filename_component (TRANS_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../translations/ ABSOLUTE)

message (STATUS "SRC_DIR: ${SRC_DIR}")
message (STATUS "DEPS_DIR: ${DEPS_DIR}")
message (STATUS "TRANS_DIR: ${TRANS_DIR}")

include_directories (${SRC_DIR} ${DEPS_DIR})

##################################################
# Build identification: git hash
#
# LaunchyVersion.h is bumped by hand, so a binary built from an untagged commit
# reports whatever the last release was -- which is exactly what a CI artifact
# cannot afford to do. The About page therefore also shows the commit the build
# came from.
#
# The hash is resolved during EVERY build by src/cmake/GenerateBuildInfo.cmake,
# not at configure time: committing does not re-run cmake, so a configure time
# value silently keeps naming the commit that was checked out when cmake last ran
# (a "-dirty" from an old configure survives a clean commit). The script locates
# git itself, writes the value into a generated header and prints it in the
# build log:
#
#   Launchy build info: git hash 8e1a4d0d (clean)
#
# The header is only rewritten when the value changed, so a build of unchanged
# sources neither recompiles nor relinks.
#
# INVARIANT: no build rule may write into the checkout. The hash is only as
# honest as the tree the probe sees, and BuildInfo.Generate runs concurrently
# with every other target the executable depends on -- a target that rewrites a
# tracked file (the .ts refresh used to, see the translation section) can dirty
# the tree before the probe runs, and then a pristine CI checkout reports
# "<hash>-dirty" for a commit that is not dirty at all. Generated files belong
# in the build tree.
#
# -DLAUNCHY_GIT_HASH=<hash> supplies the value when the tree carries no git
#  metadata, e.g. a source tarball.
set (LAUNCHY_GIT_HASH "" CACHE STRING
  "Override the git hash recorded in the build (default: resolved from the checkout)")

# Build the unit tests (QTest + CTest). OFF by default: they need the Qt5 Test
# component installed and are only wanted on machines/CI that actually run
# them. Pass -DLAUNCHY_BUILD_TESTS=ON to enable (see the test section below).
option (LAUNCHY_BUILD_TESTS "Build the unit tests (QTest + CTest)" OFF)

set (LAUNCHY_BUILD_INFO_HEADER ${CMAKE_BINARY_DIR}/generated/LaunchyBuildInfo.h)
set (LAUNCHY_BUILD_INFO_SCRIPT ${SRC_DIR}/cmake/GenerateBuildInfo.cmake)

add_custom_target (BuildInfo.Generate
  COMMAND ${CMAKE_COMMAND}
    "-DREPO_DIR=${SRC_DIR}/.."
    "-DOUTPUT_FILE=${LAUNCHY_BUILD_INFO_HEADER}"
    "-DGIT_HASH_OVERRIDE=${LAUNCHY_GIT_HASH}"
    -P "${LAUNCHY_BUILD_INFO_SCRIPT}"
  COMMENT "resolve git hash for this build"
  VERBATIM)

##################################################
# Source lists
#
# Every target below names its files explicitly -- no file(GLOB) for build
# inputs. A glob is evaluated at configure time only, so a newly added (or
# removed) .cpp/.h/.ui/.qrc/.png is silently missing from the generated build
# system until something forces cmake to re-run, and nothing does. Adding or
# removing a file therefore means editing the matching list here.
#
# The python plugins section keeps its globs on purpose: they discover plugins
# from the folder layout (a new python plugin needs no CMake change) and copy
# runtime assets -- neither is a build input.

##################################################
# Qt
# add CMAKE_PREFIX_PATH = <qt installed dir> in system PATH
# eg c:/Qt/Qt5.12.10/5.12.10/msvc2017
set (QT_MAJOR_VERSION 5)
if (WIN32)
  if (NOT DEFINED ENV{CMAKE_PREFIX_PATH})
    if (QT_MAJOR_VERSION EQUAL 5)
      set (ENV{CMAKE_PREFIX_PATH} "C:/Qt/Qt5.12.10/5.12.10/msvc2017_64")
    else ()
      set (ENV{CMAKE_PREFIX_PATH} "D:/SDK/Qt/6.5.3/msvc2019_64")
    endif ()
  endif ()
  message (STATUS "CMAKE_PREFIX_PATH $ENV{CMAKE_PREFIX_PATH}")
  find_package (Qt${QT_MAJOR_VERSION} COMPONENTS Widgets Network LinguistTools WinExtras REQUIRED)
else ()
  find_package (Qt${QT_MAJOR_VERSION} COMPONENTS Widgets Network LinguistTools REQUIRED)
  find_package (PkgConfig REQUIRED)
  pkg_check_modules (X11 x11 xcb REQUIRED)
endif ()

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

##################################################
# Python
#
# launchy.pyd is a C extension for the interpreter that ships with the app
# (python36.dll / python36.lib from the 3.6.8 install), so the interpreter used
# here has to be that same one -- not whichever Python is newest on PATH.
# FindPython's default strategy is VERSION: it picks the newest Python found in
# ANY location and treats Python_ROOT_DIR as only a hint, so on a machine (or
# CI image) that also ships a much newer interpreter the hint loses and the
# result is an artifact that links a python3x.dll it never ships. Two things
# pin it: locational search (Python_FIND_STRATEGY LOCATION plus CMP0094 NEW,
# which the policy needs because it only turned NEW in cmake 3.15 while this
# project still declares 3.14) and an explicit Python_EXECUTABLE hint, which is
# the highest priority hint of all.
#
# Python_ROOT_DIR is expected from the CI workflow (-DPython_ROOT_DIR=<setup
# python install>); C:/Program Files/Python36 is the default developer install.
if (WIN32)
  if (NOT Python_ROOT_DIR AND EXISTS "C:/Program Files/Python36/python.exe")
    set (Python_ROOT_DIR "C:/Program Files/Python36")
  endif ()
  if (Python_ROOT_DIR)
    set (Python_EXECUTABLE "${Python_ROOT_DIR}/python.exe")
  endif ()
  set (Python_FIND_STRATEGY LOCATION)
  if (POLICY CMP0094)
    cmake_policy (SET CMP0094 NEW)
  endif ()
endif ()

find_package (Python COMPONENTS Interpreter Development REQUIRED)

# Python_VERSION_STRING carries major.minor.patch, Python_VERSION only
# major.minor on older FindPython versions.
set (LAUNCHY_PYTHON_VERSION "${Python_VERSION_STRING}")
if (NOT LAUNCHY_PYTHON_VERSION)
  set (LAUNCHY_PYTHON_VERSION "${Python_VERSION}")
endif ()

if (WIN32)
  # Fail here rather than ship a .pyd that imports a DLL the artifact lacks
  if (NOT LAUNCHY_PYTHON_VERSION MATCHES "^3\\.6\\.")
    message (FATAL_ERROR
      "launchy.pyd must be built against the embedded Python 3.6 (the runtime "
      "next to Launchy.exe is python36.dll), found ${LAUNCHY_PYTHON_VERSION} at "
      "${Python_EXECUTABLE}. Pass -DPython_ROOT_DIR=<python install> to point "
      "at the 3.6.8 one.")
  endif ()
  if (NOT LAUNCHY_PYTHON_VERSION STREQUAL "3.6.8")
    message (WARNING
      "building against Python ${LAUNCHY_PYTHON_VERSION}, the embedded runtime "
      "is 3.6.8")
  endif ()
endif ()

message ("Python_FOUND: ${Python_FOUND}")
message ("Python_VERSION: ${Python_VERSION}")
message ("Python_VERSION_STRING: ${Python_VERSION_STRING}")
message ("Python_EXECUTABLE: ${Python_EXECUTABLE}")
message ("Python_Development_FOUND: ${Python_Development_FOUND}")
message ("Python_INCLUDE_DIRS: ${Python_INCLUDE_DIRS}")
message ("Python_LIBRARIES: ${Python_LIBRARIES}")

##################################################
# Platform
if (MSVC)
  add_compile_options (/W3 /utf-8)
  # add_compile_options (/wd4819) # disable warning C4819: unicode charset
  # add_compile_options (/MP) # multi-processor compilation

  # generate debug info (pdb files)
  add_compile_options (/Zi)
  add_link_options (/DEBUG)

  add_compile_definitions (_CRT_SECURE_NO_WARNINGS)
  add_compile_definitions (NOMINMAX)
  # add_compile_definitions (WIN32_LEAN_AND_MEAN)

#  add_link_options (/DEBUG) # generate debug info in release config

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
if (WIN32)
  set_target_properties (Launchy PROPERTIES WIN32_EXECUTABLE TRUE)
endif ()

# source files
set (SOURCES_LAUNCHY
  ${SRC_DIR}/Launchy/AnimationLabel.h
  ${SRC_DIR}/Launchy/AnimationLabel.cpp
  ${SRC_DIR}/Launchy/CharLineEdit.h
  ${SRC_DIR}/Launchy/CharLineEdit.cpp
  ${SRC_DIR}/Launchy/CharListWidget.h
  ${SRC_DIR}/Launchy/CharListWidget.cpp
  ${SRC_DIR}/Launchy/DropListWidget.h
  ${SRC_DIR}/Launchy/DropListWidget.cpp
  ${SRC_DIR}/Launchy/Fader.h
  ${SRC_DIR}/Launchy/Fader.cpp
  ${SRC_DIR}/Launchy/FileBrowser.h
  ${SRC_DIR}/Launchy/FileBrowser.cpp
  ${SRC_DIR}/Launchy/FileBrowserDelegate.h
  ${SRC_DIR}/Launchy/FileBrowserDelegate.cpp
  ${SRC_DIR}/Launchy/FileSearch.h
  ${SRC_DIR}/Launchy/FileSearch.cpp
  ${SRC_DIR}/Launchy/IconDelegate.h
  ${SRC_DIR}/Launchy/IconDelegate.cpp
  ${SRC_DIR}/Launchy/LaunchyApp.h
  ${SRC_DIR}/Launchy/LaunchyApp.cpp
  ${SRC_DIR}/Launchy/LaunchyVersion.h
  ${SRC_DIR}/Launchy/LaunchyWidget.h
  ${SRC_DIR}/Launchy/LaunchyWidget.cpp
  ${SRC_DIR}/Launchy/MemProfiler.h
  ${SRC_DIR}/Launchy/MemProfiler.cpp
  ${SRC_DIR}/Launchy/main.cpp
  ${SRC_DIR}/Launchy/OptionDialog.h
  ${SRC_DIR}/Launchy/OptionDialog.cpp
  ${SRC_DIR}/Launchy/PluginFactory.h
  ${SRC_DIR}/Launchy/PluginFactory.cpp
  ${SRC_DIR}/Launchy/TranslationManager.h
  ${SRC_DIR}/Launchy/TranslationManager.cpp
  ${SRC_DIR}/Launchy/UpdateChecker.h
  ${SRC_DIR}/Launchy/UpdateChecker.cpp
)


# Sources shared by every platform: the icon cache base, the worker that fills
# it in the background, and the platform seam itself (PlatformBase, the installed
# instance and the factory). Together with the platform specific files below they
# build the LaunchyPlatform library, which answers every question the core asks
# the OS and produces the QIcons the UI needs -- IconExtractor is here because
# deciding which file an item's icon comes from is an OS question, not a UI one.
# The seam lives here rather than in LaunchyCore because only this library knows
# how to create a platform: with both in one place, the installed pointer is a
# file-local in Platform.cpp instead of a global in a header, and nothing has to
# be inline. The application object (LaunchyApp, a QApplication) is deliberately NOT
# here: it lives in the executable so this library stays free of QApplication.
set (SOURCES_PLATFORM_COMMON
  ${SRC_DIR}/Platform/IconExtractor.h
  ${SRC_DIR}/Platform/IconExtractor.cpp
  ${SRC_DIR}/Platform/IconProviderBase.h
  ${SRC_DIR}/Platform/IconProviderBase.cpp
  ${SRC_DIR}/Platform/Platform.h
  ${SRC_DIR}/Platform/Platform.cpp)

# sources files for operation system
if (WIN32)
  set (SOURCES_PLATFORM
    ${SRC_DIR}/Platform/Windows/PlatformWin.h
    ${SRC_DIR}/Platform/Windows/PlatformWin.cpp
    ${SRC_DIR}/Platform/Windows/CrashDumper.h
    ${SRC_DIR}/Platform/Windows/CrashDumper.cpp
    ${SRC_DIR}/Platform/Windows/IconProviderWin.h
    ${SRC_DIR}/Platform/Windows/IconProviderWin.cpp
    ${SRC_DIR}/Platform/Windows/UtilWin.h
    ${SRC_DIR}/Platform/Windows/UtilWin.cpp)
  # The main widget override and the exe resource stay in the executable:
  # LaunchyWidgetWin inherits the application's LaunchyWidget, launchy.rc is the
  # exe resource. There is no per-platform application class any more (AppWin
  # was an empty LaunchyApp once the crash dumper moved to PlatformWin), so the
  # QApplication itself is built by initApplication() in Launchy/LaunchyApp.cpp.
  set (SOURCES_PLATFORM_LAUNCHY
    ${SRC_DIR}/Launchy/Windows/LaunchyWidgetWin.h
    ${SRC_DIR}/Launchy/Windows/LaunchyWidgetWin.cpp
    ${SRC_DIR}/Launchy/Windows/launchy.rc)
  source_group ("Resource Files" FILES ${SRC_DIR}/Launchy/Windows/launchy.rc)
else ()
  set (SOURCES_PLATFORM
    ${SRC_DIR}/Platform/Linux/PlatformLinux.h
    ${SRC_DIR}/Platform/Linux/PlatformLinux.cpp
    ${SRC_DIR}/Platform/Linux/IconProviderLinux.h
    ${SRC_DIR}/Platform/Linux/IconProviderLinux.cpp)
  set (SOURCES_PLATFORM_LAUNCHY
    ${SRC_DIR}/Launchy/Linux/LaunchyWidgetLinux.h
    ${SRC_DIR}/Launchy/Linux/LaunchyWidgetLinux.cpp)
endif (WIN32)

list (APPEND SOURCES_LAUNCHY ${SOURCES_PLATFORM_LAUNCHY})
target_include_directories (Launchy PRIVATE ${SRC_DIR}/Launchy)

# qt source files
set (SOURCES_QT_LAUNCHY
  ${SRC_DIR}/Launchy/OptionDialog.ui
  ${SRC_DIR}/Launchy/launchy.qrc)
source_group ("Qt Files" FILES ${SOURCES_QT_LAUNCHY})
list (APPEND SOURCES_LAUNCHY ${SOURCES_QT_LAUNCHY})

target_sources (Launchy
  PRIVATE ${SOURCES_LAUNCHY}
  PRIVATE ${SOURCES_SINGLEAPP}
  PRIVATE ${SOURCES_HOTKEY})

# The commit this build came from. BuildInfo.Generate writes the generated
# header before any Launchy source is compiled; the include dir is what lets
# LaunchyVersion.h find it.
add_dependencies (Launchy BuildInfo.Generate)
target_include_directories (Launchy PRIVATE ${CMAKE_BINARY_DIR}/generated)

if (WIN32)
target_link_libraries (Launchy
  PluginInterface LaunchyCore LaunchyPlatform PluginPy
  SingleApplication QHotKey
  Qt${QT_MAJOR_VERSION}::Widgets Qt${QT_MAJOR_VERSION}::Network Qt${QT_MAJOR_VERSION}::WinExtras)
else ()
target_link_libraries (Launchy
  PluginInterface LaunchyCore LaunchyPlatform PluginPy
  SingleApplication QHotKey
  Qt${QT_MAJOR_VERSION}::Widgets Qt${QT_MAJOR_VERSION}::Network Qt${QT_MAJOR_VERSION}::GuiPrivate)
endif ()

##################################################
# OpenSSL runtime for Qt Network (https update checker)
# Qt 5.12 loads OpenSSL at runtime (libssl-1_1-x64.dll / libcrypto-1_1-x64.dll),
# without them https requests fail with "network error 99: TLS initialization failed"
if (WIN32)
  set (OPENSSL_RUNTIME_DIR ${DEPS_DIR}/openssl)
  set (OPENSSL_RUNTIME_DLLS
    ${OPENSSL_RUNTIME_DIR}/libssl-1_1-x64.dll
    ${OPENSSL_RUNTIME_DIR}/libcrypto-1_1-x64.dll)

  set (OPENSSL_RUNTIME_MISSING FALSE)
  foreach (OPENSSL_RUNTIME_DLL ${OPENSSL_RUNTIME_DLLS})
    if (NOT EXISTS "${OPENSSL_RUNTIME_DLL}")
      set (OPENSSL_RUNTIME_MISSING TRUE)
    endif ()
  endforeach ()

  if (OPENSSL_RUNTIME_MISSING)
    message (WARNING "OpenSSL runtime DLLs not found in ${OPENSSL_RUNTIME_DIR}, "
      "https update check will fail with: TLS initialization failed")
  else ()
    # The destination is the directory Launchy.exe actually lands in
    # ($<TARGET_FILE_DIR:Launchy>), not CMAKE_RUNTIME_OUTPUT_DIRECTORY/$<CONFIG>:
    # the two agree today, but the latter silently goes stale the moment the
    # target overrides its own RUNTIME_OUTPUT_DIRECTORY, and the DLLs have to
    # sit next to the executable for the loader to find them.
    add_custom_command (TARGET Launchy POST_BUILD
      COMMAND ${CMAKE_COMMAND} -E copy_if_different
      ${OPENSSL_RUNTIME_DLLS} "$<TARGET_FILE_DIR:Launchy>"
      COMMENT "copy OpenSSL runtime DLLs to target directory")
  endif ()
endif ()

##################################################
# PluginInterface
# source files
add_library (PluginInterface SHARED)

target_sources (PluginInterface
  PRIVATE
  ${SRC_DIR}/PluginInterface/CatalogItem.h
  ${SRC_DIR}/PluginInterface/CatalogItem.cpp
  ${SRC_DIR}/PluginInterface/InputData.h
  ${SRC_DIR}/PluginInterface/InputData.cpp
  ${SRC_DIR}/PluginInterface/Runtime.h
  ${SRC_DIR}/PluginInterface/Runtime.cpp
  ${SRC_DIR}/PluginInterface/PluginInterface.h
  ${SRC_DIR}/PluginInterface/PluginInterface.cpp
  ${SRC_DIR}/PluginInterface/PluginMsg.h
  ${SRC_DIR}/PluginInterface/UnicodeTable.h
  ${SRC_DIR}/PluginInterface/UnicodeTable.cpp)

# Qt Core ONLY. This library is the plugin ABI: it ships next to every plugin,
# to python/launchy.pyd and to the headless unit tests, so whatever it imports
# has to be present in all of those directories -- and the test tree only copies
# Qt5Core.dll and Qt5Test.dll. Widgets used to be linked here "just in case"; no
# header or source of this library names a widget type (the only mentions are
# inside PluginMsg.h's doc comments), so it produced no import entry and only
# advertised a dependency that did not exist. Do not add it back: the day a real
# widget symbol is used, PluginInterface.dll starts importing Qt5Widgets.dll and
# every test binary fails at startup with 0xc0000135. Needing a QWidget* in the
# ABI is fine -- forward declare it or pass void*, as msg() already does.
target_link_libraries (PluginInterface
  PRIVATE Qt${QT_MAJOR_VERSION}::Core)

##################################################
# LaunchyCore
#
# Domain logic that must be reachable without a GUI: the catalogs, their
# ranking and the option keys. STATIC on purpose --
#
#   * its only consumers live in this tree (the executable, and the unit
#     tests), so a DLL would add a deploy step and a second ABI seam that
#     nothing needs: the app tree is updated atomically,
#   * everything that links it carries a private copy of the code, which is
#     only harmless for stateless code. A stateful singleton here (the plugin
#     registry, the command history) must therefore have exactly one linking
#     consumer that owns it: the executable. PluginPy keeps to the stateless
#     parts -- model types, ranking -- and talks to the app through the plugin
#     message ABI, never through a core object.
#
# Rule for anything added here: Qt Core only. No QWidget, no qApp, no platform
# headers, no platform implementation. If it needs those, it belongs to
# the application, not here. That rule is what lets test/ link this library
# and exercise the logic headless.
add_library (LaunchyCore STATIC)

target_sources (LaunchyCore
  PRIVATE
  ${SRC_DIR}/Core/Catalog.h
  ${SRC_DIR}/Core/Catalog.cpp
  ${SRC_DIR}/Core/CatalogBuilder.h
  ${SRC_DIR}/Core/CatalogBuilder.cpp
  ${SRC_DIR}/Core/CatalogFast.h
  ${SRC_DIR}/Core/CatalogFast.cpp
  ${SRC_DIR}/Core/CatalogSlow.h
  ${SRC_DIR}/Core/CatalogSlow.cpp
  ${SRC_DIR}/Core/CatalogRanking.h
  ${SRC_DIR}/Core/CatalogRanking.cpp
  ${SRC_DIR}/Core/CommandHistory.h
  ${SRC_DIR}/Core/CommandHistory.cpp
  ${SRC_DIR}/Core/CoreGlobals.h
  ${SRC_DIR}/Core/CoreGlobals.cpp
  ${SRC_DIR}/Core/Directory.h
  ${SRC_DIR}/Core/Directory.cpp
  ${SRC_DIR}/Core/InputDataList.h
  ${SRC_DIR}/Core/InputDataList.cpp
  ${SRC_DIR}/Core/Logger.h
  ${SRC_DIR}/Core/Logger.cpp
  ${SRC_DIR}/Core/OptionItem.h
  ${SRC_DIR}/Core/OptionItem.cpp
  ${SRC_DIR}/Core/PluginInfo.h
  ${SRC_DIR}/Core/PluginInfo.cpp
  ${SRC_DIR}/Core/PluginRegistry.h
  ${SRC_DIR}/Core/PluginRegistry.cpp
  ${SRC_DIR}/Core/SettingsManager.h
  ${SRC_DIR}/Core/SettingsManager.cpp
  ${SRC_DIR}/Core/StartupOption.h
  ${SRC_DIR}/Core/StartupOption.cpp
  ${SRC_DIR}/Core/VersionTag.h
  ${SRC_DIR}/Core/VersionTag.cpp)

# PUBLIC: consumers of the catalogs also need the model types' include root
# ("PluginInterface/CatalogItem.h") and the same option keys.
#
# ${SRC_DIR}/Core is deliberately NOT an include directory, for either side:
#
#   * a consumer of the library (the executable, the platform library, the
#     tests) writes the path it sees from src/: "Core/Catalog.h",
#   * core's own sources include their siblings bare ("Catalog.h"), exactly as
#     PluginInterface does ("Runtime.h"). That works without any include
#     directory because a quoted include is searched in the directory of the
#     including file first -- and every core file lives in src/Core.
#
# The point of the split is that one name can never resolve to a header of
# another layer by accident, and the include site says which layer a symbol
# comes from. Do not add ${SRC_DIR}/Core (PUBLIC or PRIVATE) back.
target_include_directories (LaunchyCore
  PUBLIC ${SRC_DIR})

# PluginInterface carries the model types (CatItem, InputData) and the plugin ABI,
# so core links it rather than duplicating the classes.
#
# Network is PRIVATE and compile-time only: OptionItem.cpp needs the
# QNetworkProxy enumerator for one default value (OPTION_PROXY_TYPE_DEFAULT).
# An enumerator leaves no symbol reference, so nothing pulling core in also
# imports Qt5Network.dll. Do not use this as a licence to put network code in
# core -- the proxy settings themselves are read by the application.
target_link_libraries (LaunchyCore
  PUBLIC PluginInterface Qt${QT_MAJOR_VERSION}::Core
  PRIVATE Qt${QT_MAJOR_VERSION}::Network)

##################################################
# LaunchyPlatform
#
# The operating system differences, gathered in one library. Everything that
# answers a platform question (directories, environment, idle time, network,
# running a program, the icon cache) lives here, in the platform specific
# subfolders (Platform/Windows, Platform/Linux), together with the seam that
# hands the answer to the rest of the tree (Platform/Platform.h). It links
# LaunchyCore for the model types and the logging helpers.
#
# STATIC, like LaunchyCore, because its only consumers are the executable and
# the unit tests in this tree -- a DLL would add a deploy step and an ABI seam
# nothing needs. It links Qt Widgets/Gui for the icon cache (QFileIconProvider /
# QIcon), but the code does not use QApplication -- the application object lives
# in the executable, which is what keeps this library free of QApplication.
add_library (LaunchyPlatform STATIC)
target_sources (LaunchyPlatform
  PRIVATE ${SOURCES_PLATFORM_COMMON}
  PRIVATE ${SOURCES_PLATFORM})

target_include_directories (LaunchyPlatform
  PUBLIC ${SRC_DIR}/Platform ${SRC_DIR})

if (WIN32)
  # WinExtras for the taskbar, and the two system libraries the platform answers
  # need: userenv (CreateEnvironmentBlock) and netapi32 (NetServerEnum). They are
  # declared here, where the calls live, so anything linking this library gets
  # them whether or not it is the executable.
  target_link_libraries (LaunchyPlatform
    PRIVATE Qt${QT_MAJOR_VERSION}::WinExtras
    PRIVATE userenv netapi32)
else ()
  # LINUX
endif ()

# Widgets + Gui are PRIVATE: Platform.h is Qt Core only, and nothing it declares
# mentions a GUI type. The GUI dependency is still real -- the icon providers
# derive from QFileIconProvider and produce QIcons -- but it stays inside this
# library's own sources, so a consumer of the seam (the headless tests included)
# neither needs the include paths nor Qt5Gui.dll at run time. IconExtractor.h
# does expose QIcon; its only consumer is the widget, a Qt Widgets application.
# Exposing a type is not the same as using a widget: no header of this library
# declares one, and QFileIconProvider needs the QtWidgets library, not a
# QApplication instance.
target_link_libraries (LaunchyPlatform
  PUBLIC LaunchyCore
  PRIVATE Qt${QT_MAJOR_VERSION}::Widgets Qt${QT_MAJOR_VERSION}::Gui)

##################################################
# PluginPy
add_library (PluginPy SHARED)

target_sources (PluginPy
  PRIVATE
  ${SRC_DIR}/PluginPy/ExportPy.cpp
  ${SRC_DIR}/PluginPy/ExportPyCatItem.h
  ${SRC_DIR}/PluginPy/ExportPyCatItem.cpp
  ${SRC_DIR}/PluginPy/ExportPyInputData.h
  ${SRC_DIR}/PluginPy/ExportPyInputData.cpp
  ${SRC_DIR}/PluginPy/ExportPyPlugin.h
  ${SRC_DIR}/PluginPy/PluginLoader.h
  ${SRC_DIR}/PluginPy/PluginLoader.cpp
  ${SRC_DIR}/PluginPy/PluginPy.h
  ${SRC_DIR}/PluginPy/PluginWrapper.h
  ${SRC_DIR}/PluginPy/PluginWrapper.cpp)

target_include_directories (PluginPy
  PRIVATE ${Python_INCLUDE_DIRS})

# Qt Core only, for the same reason as PluginInterface: this is the python face
# of the plugin ABI, deployed as python/launchy.pyd, and whatever it imports has
# to exist wherever it is loaded. It does handle a dialog parent, but only as an
# opaque pointer handed straight back to the plugin (PluginWrapper.h forward
# declares QWidget), and it reaches the application object through
# qApp->applicationDirPath() / QCoreApplication::aboutToQuit -- both Qt Core, so
# the Widgets link produced no import entry at all. Keep it that way: including a
# QtWidgets header here would make PluginPy.dll (and tests that load it) fail to
# start whenever Qt5Widgets.dll is not next to them.
target_link_libraries (PluginPy
  PUBLIC PluginInterface
  PRIVATE Qt${QT_MAJOR_VERSION}::Core
  PRIVATE ${Python_LIBRARIES})

# Copy the built plugin and its python helper into the target's python/ dir on
# every build (ALL), so the run/deploy tree always has a matching launchy.pyd
# and launchy_util.py. DEPENDS PluginPy guarantees the .pyd exists before copy.
add_custom_target (PluginPy.Deploy ALL
  COMMAND ${CMAKE_COMMAND} -E make_directory
  ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/$<CONFIG>/python
  COMMAND ${CMAKE_COMMAND} -E copy_if_different
  $<TARGET_FILE:PluginPy>
  ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/$<CONFIG>/python/launchy.pyd
  COMMAND ${CMAKE_COMMAND} -E copy_if_different
  ${SRC_DIR}/PluginPy/launchy_util.py
  ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/$<CONFIG>/python
  COMMENT "copy PluginPy -> python/launchy.pyd and launchy_util.py to target directory"
  DEPENDS PluginPy
  SOURCES ${SRC_DIR}/PluginPy/launchy_util.py)

##################################################
# python plugins
#
# PluginRegistry::loadPlugins() scans the plugins/ directory and treats
# plugins/<Name>/ as a plugin whenever <Name>.py exists inside it. The native
# plugins reach that folder through their LIBRARY_OUTPUT_DIRECTORY_RELEASE,
# but a python plugin has nothing to link, so its whole source folder used to
# be copied there by hand. This target does it on every build instead, and it
# picks up any folder under src/Plugins/ that owns a <FolderName>.py, so adding
# a new python plugin needs no CMake change.
#
# The whole folder is copied (entries, icons, data.json, helper packages such
# as Calcy/ or WebSearch/) because the plugins read their assets at runtime.
# Only __pycache__ is dropped -- it is generated whenever the plugins run.
#
# The copy is forced to run after every C++ target at the bottom of this file,
# see the "python plugins: copy them after the C++ targets" section.
#
# This glob and the GLOB_RECURSE below are the deliberate exception to the
# no-glob rule at the top of this file: they discover plugins from the folder
# layout instead of listing build inputs, and CONFIGURE_DEPENDS makes the
# generator re-run when a plugin folder appears or disappears.
file (GLOB PY_PLUGIN_DIRS CONFIGURE_DEPENDS ${SRC_DIR}/Plugins/*)

# A folder counts as a python plugin when it owns a <FolderName>.py, the rule
# PluginRegistry::loadPlugins() uses to scan the plugins/ directory. Collecting
# them first also gives the total for the "[i/n]" progress prefix below.
set (PYTHON_PLUGINS "")
foreach (pyPluginDir ${PY_PLUGIN_DIRS})
  if (NOT IS_DIRECTORY "${pyPluginDir}")
    continue ()
  endif ()
  get_filename_component (pyPluginName "${pyPluginDir}" NAME)
  if (EXISTS "${pyPluginDir}/${pyPluginName}.py")
    list (APPEND PYTHON_PLUGINS "${pyPluginName}")
  endif ()
endforeach ()

list (LENGTH PYTHON_PLUGINS PYTHON_PLUGIN_COUNT)
message (STATUS "python plugins (${PYTHON_PLUGIN_COUNT}): ${PYTHON_PLUGINS}")

add_custom_target (PythonPlugins.Deploy ALL
  COMMAND ${CMAKE_COMMAND} -E echo "copy python plugins -> ${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/$<CONFIG>/plugins"
  COMMENT "copy python plugins into target directory")

set (pyPluginIndex 0)

foreach (pyPluginName ${PYTHON_PLUGINS})
  math (EXPR pyPluginIndex "${pyPluginIndex} + 1")

  set (pyPluginDir "${SRC_DIR}/Plugins/${pyPluginName}")
  set (pyPluginDest "${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/$<CONFIG>/plugins/${pyPluginName}")

  # __pycache__ sits at the plugin root and inside every packaged helper folder
  # (Calcy/, Diry/, WebSearch/), so the source paths are rewritten to their
  # counterparts in the output tree. GLOB_RECURSE keeps this independent of
  # how deep a helper package nests. Like the glob above it walks runtime data,
  # not build inputs, so it stays.
  file (GLOB_RECURSE pyPluginEntries "${pyPluginDir}/**")

  # Progress goes through echo (a real command), not through COMMENT: the
  # MSBuild generator batches every POST_BUILD comment of a target onto a
  # single tab separated line, which glued all plugins together. Each echo
  # owns its own stdout line instead. The counters describe what lands in the
  # output tree, so cache folders are excluded from the file count and only
  # counted separately.
  set (pyPluginFileCount 0)
  set (pyPluginCacheArgs "")
  foreach (pyPluginEntry ${pyPluginEntries})
    if (pyPluginEntry MATCHES "/__pycache__/")
      string (REPLACE "${pyPluginDir}" "${pyPluginDest}" pyCacheDest "${pyPluginEntry}")
      get_filename_component (pyCacheDirDest "${pyCacheDest}" DIRECTORY)
      list (APPEND pyPluginCacheArgs "${pyCacheDirDest}")
      continue ()
    endif ()
    if (NOT IS_DIRECTORY "${pyPluginEntry}")
      math (EXPR pyPluginFileCount "${pyPluginFileCount} + 1")
    endif ()
  endforeach ()
  list (REMOVE_DUPLICATES pyPluginCacheArgs)
  list (LENGTH pyPluginCacheArgs pyPluginCacheCount)

  add_custom_command (TARGET PythonPlugins.Deploy POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E echo "  [${pyPluginIndex}/${PYTHON_PLUGIN_COUNT}] ${pyPluginName}: copying ${pyPluginFileCount} files, dropping ${pyPluginCacheCount} __pycache__ -> plugins/${pyPluginName}"
    COMMAND ${CMAKE_COMMAND} -E copy_directory "${pyPluginDir}" "${pyPluginDest}"
    VERBATIM)

  # remove_directory takes a single directory on CMake < 3.17, so one command
  # per cache folder instead of passing the whole list at once.
  foreach (pyCacheDirDest ${pyPluginCacheArgs})
    add_custom_command (TARGET PythonPlugins.Deploy POST_BUILD
      COMMAND ${CMAKE_COMMAND} -E remove_directory "${pyCacheDirDest}"
      VERBATIM)
  endforeach ()
endforeach ()

##################################################
# Launchy main translation
# Adding a language means adding its .ts here as well.
set (TS_FILES_LAUNCHY
  ${TRANS_DIR}/launchy_de.ts
  ${TRANS_DIR}/launchy_es.ts
  ${TRANS_DIR}/launchy_fr.ts
  ${TRANS_DIR}/launchy_it.ts
  ${TRANS_DIR}/launchy_ja.ts
  ${TRANS_DIR}/launchy_nl.ts
  ${TRANS_DIR}/launchy_pt.ts
  ${TRANS_DIR}/launchy_ru.ts
  ${TRANS_DIR}/launchy_zh_CN.ts
  ${TRANS_DIR}/launchy_zh_TW.ts)

if (QT_MAJOR_VERSION EQUAL 5)

  # Compile the .qm files only. The committed .ts files are the INPUT of the
  # build, not a build product -- and qt5_create_translation would have made
  # them a product: it is lupdate + lrelease, so every build where a translatable
  # source is newer than the .ts rewrites translations/*.ts in the checkout
  # (lupdate refreshes every <location line="..."> even when no string changed).
  #
  # That in-place rewrite is what put "-dirty" into CI artifacts. A fresh clone
  # always has locations stale relative to the sources, so every CI build
  # rewrote the .ts files, and BuildInfo.Generate (which asks git for the tree
  # state) is an INDEPENDENT dependency of Launchy -- with --parallel the two run
  # concurrently, so the rewrite could easily land before the git probe and a
  # pristine tree was reported as dirty. Refreshing the .ts therefore happens in
  # a separate target below, never during a normal build.
  qt5_add_translation (QM_FILES_LAUNCHY ${TS_FILES_LAUNCHY})

  # Drive the .qm generation and copy them next to the executable.
  add_custom_target (Launchy.Translations ALL DEPENDS ${QM_FILES_LAUNCHY})
  add_dependencies (Launchy Launchy.Translations)

  add_custom_command (TARGET Launchy.Translations POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory
    ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/$<CONFIG>/translations
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
    ${QM_FILES_LAUNCHY}
    ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/$<CONFIG>/translations
    COMMENT "copy generated .qm translation files to translations directory")

  # Deliberately NOT part of ALL, for the reason given above. Run it by hand
  # after touching translatable code, then review and commit the .ts diff --
  # locations move even when no string changed, which is exactly why the result
  # must not appear in a build's working tree.
  #
  #   cmake --build <build> --target Launchy.UpdateTranslations
  add_custom_target (Launchy.UpdateTranslations
    COMMAND ${Qt5_LUPDATE_EXECUTABLE}
      ${SRC_DIR}/Launchy
      ${SRC_DIR}/PluginInterface
      ${SRC_DIR}/Core
      ${SRC_DIR}/PluginPy
      -ts ${TS_FILES_LAUNCHY}
    COMMENT "refresh translations/*.ts from the sources (review and commit the diff)"
    VERBATIM)

else ()

  qt_add_lupdate (Launchy
    TS_FILES ${TS_FILES_LAUNCHY}
    OPTIONS NO_GLOBAL_TARGET)

  qt_add_lrelease (Launchy
    TS_FILES ${TS_FILES_LAUNCHY}
    QM_FILES_OUTPUT_VARIABLE QM_FILES_LAUNCHY
    OPTIONS NO_GLOBAL_TARGET)

  add_custom_command (TARGET Launchy_lrelease POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory
    ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/$<CONFIG>/translations
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
    ${QM_FILES_LAUNCHY}
    ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/$<CONFIG>/translations
    COMMENT "copy generated .qm translation files to translations directory")

endif () # QT_MAJOR_VERSION

# message (STATUS "QM_FILES_LAUNCHY: ${QM_FILES_LAUNCHY}")


##################################################
# QSingleApplication
set(QAPPLICATION_CLASS QApplication)
add_subdirectory(${DEPS_DIR}/SingleApplication SingleApplication)

##################################################
# QHotKey
add_library (QHotKey STATIC)

target_sources (QHotKey PRIVATE ${DEPS_DIR}/QHotkey/QHotkey.cpp)

if (WIN32)
  target_sources(QHotKey PRIVATE ${DEPS_DIR}/QHotkey/qhotkey_win.cpp)
else ()
  find_package(X11 REQUIRED)
  include_directories(${X11_INCLUDE_DIR})
  target_link_libraries(QHotKey PRIVATE ${X11_LIBRARIES})
  target_sources(QHotKey PRIVATE ${DEPS_DIR}/QHotkey/qhotkey_x11.cpp)
endif ()

if (WIN32)
target_link_libraries (QHotKey
  PUBLIC Qt${QT_MAJOR_VERSION}::Core
  PRIVATE Qt${QT_MAJOR_VERSION}::Widgets)
else ()
target_link_libraries (QHotKey
  PUBLIC Qt${QT_MAJOR_VERSION}::Core
  PRIVATE Qt${QT_MAJOR_VERSION}::Widgets X11 xcb)
endif ()

##################################################
# plugin: Runner
add_library (Plugin.Runner MODULE)

set_target_properties(Plugin.Runner PROPERTIES
  OUTPUT_NAME "Runner")

set (SOURCES_RUNNER
  ${SRC_DIR}/Plugins/Runner/Runner.h
  ${SRC_DIR}/Plugins/Runner/Runner.cpp
  ${SRC_DIR}/Plugins/Runner/Globals.h
  ${SRC_DIR}/Plugins/Runner/Globals.cpp
  ${SRC_DIR}/Plugins/Runner/gui.h
  ${SRC_DIR}/Plugins/Runner/gui.cpp)

# qt source files
set (SOURCES_QT_RUNNER
  ${SRC_DIR}/Plugins/Runner/dlg.ui)
source_group ("Qt Files" FILES ${SOURCES_QT_RUNNER})
list (APPEND SOURCES_RUNNER ${SOURCES_QT_RUNNER})

target_sources (Plugin.Runner
  PRIVATE ${SOURCES_RUNNER})

target_link_libraries (Plugin.Runner
  PUBLIC Qt${QT_MAJOR_VERSION}::Core
  PRIVATE Qt${QT_MAJOR_VERSION}::Widgets
  PRIVATE PluginInterface)

# plugin/<Name>/ is the folder PluginRegistry::loadPlugins() expects. One
# generator expression instead of a _DEBUG/_RELEASE pair: $<CONFIG> expands to
# the config being built, and this property accepts target level genexes.
set_target_properties (Plugin.Runner PROPERTIES
  LIBRARY_OUTPUT_DIRECTORY "${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/$<CONFIG>/plugins/Runner")

set (RESOURCES_RUNNER
  ${SRC_DIR}/Plugins/Runner/runner.png)

add_custom_command (TARGET Plugin.Runner POST_BUILD
  COMMAND ${CMAKE_COMMAND} -E copy_if_different
  ${RESOURCES_RUNNER} ${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/$<CONFIG>/plugins/Runner)

##################################################
if (WIN32)
# plugin: Tasky
add_library (Plugin.Tasky MODULE)

set_target_properties(Plugin.Tasky PROPERTIES
  OUTPUT_NAME "Tasky")

set (SOURCES_TASKY
  ${SRC_DIR}/Plugins/Tasky/Tasky.h
  ${SRC_DIR}/Plugins/Tasky/Tasky.cpp)

# qt source files (none yet)
set (SOURCES_QT_TASKY)
source_group ("Qt Files" FILES ${SOURCES_QT_TASKY})
list (APPEND SOURCES_TASKY ${SOURCES_QT_TASKY})

target_sources (Plugin.Tasky
  PRIVATE ${SOURCES_TASKY})

target_link_libraries (Plugin.Tasky
  PUBLIC Qt${QT_MAJOR_VERSION}::Core
  PRIVATE Qt${QT_MAJOR_VERSION}::Widgets
  PRIVATE Qt${QT_MAJOR_VERSION}::WinExtras
  PRIVATE PluginInterface)

set_target_properties (Plugin.Tasky PROPERTIES
  LIBRARY_OUTPUT_DIRECTORY "${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/$<CONFIG>/plugins/Tasky")

set (RESOURCES_TASKY
  ${SRC_DIR}/Plugins/Tasky/tasky.png)

add_custom_command (TARGET Plugin.Tasky POST_BUILD
  COMMAND ${CMAKE_COMMAND} -E copy_if_different
  ${RESOURCES_TASKY} ${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/$<CONFIG>/plugins/Tasky)
endif ()
##################################################
if (WIN32)
# plugin: UWPApp
add_library (Plugin.UWPApp MODULE)

set_target_properties(Plugin.UWPApp PROPERTIES
  OUTPUT_NAME "UWPApp")

set (SOURCES_UWPAPP
  ${SRC_DIR}/Plugins/UWPApp/UWPApp.h
  ${SRC_DIR}/Plugins/UWPApp/UWPApp.cpp)

# qt source files (none yet)
set (SOURCES_QT_UWPAPP)
source_group ("Qt Files" FILES ${SOURCES_QT_UWPAPP})
list (APPEND SOURCES_UWPAPP ${SOURCES_QT_UWPAPP})

target_sources (Plugin.UWPApp
  PRIVATE ${SOURCES_UWPAPP})

# gdi32 is needed to read the HBITMAP that IShellItemImageFactory returns for
# entries whose icon cannot be resolved inside the package (GetDIBits/DeleteObject).
target_link_libraries (Plugin.UWPApp
  PUBLIC Qt${QT_MAJOR_VERSION}::Core
  PRIVATE Qt${QT_MAJOR_VERSION}::Widgets
  PRIVATE PluginInterface
  PRIVATE user32 shell32 propsys gdi32)

set_target_properties (Plugin.UWPApp PROPERTIES
  LIBRARY_OUTPUT_DIRECTORY "${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/$<CONFIG>/plugins/UWPApp")
endif ()
##################################################
# plugin: Verby
add_library (Plugin.Verby MODULE)

set_target_properties(Plugin.Verby PROPERTIES
  OUTPUT_NAME "Verby")

set (SOURCES_VERBY
  ${SRC_DIR}/Plugins/Verby/Verby.h
  ${SRC_DIR}/Plugins/Verby/Verby.cpp
  ${SRC_DIR}/Plugins/Verby/gui.h
  ${SRC_DIR}/Plugins/Verby/gui.cpp)

# qt source files
set (SOURCES_QT_VERBY
  ${SRC_DIR}/Plugins/Verby/dlg.ui)
source_group ("Qt Files" FILES ${SOURCES_QT_VERBY})
list (APPEND SOURCES_VERBY ${SOURCES_QT_VERBY})

target_sources (Plugin.Verby
  PRIVATE ${SOURCES_VERBY})

target_link_libraries (Plugin.Verby
  PUBLIC Qt${QT_MAJOR_VERSION}::Core
  PRIVATE Qt${QT_MAJOR_VERSION}::Widgets
  PRIVATE PluginInterface)

set_target_properties (Plugin.Verby PROPERTIES
  LIBRARY_OUTPUT_DIRECTORY "${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/$<CONFIG>/plugins/Verby")

set (RESOURCES_VERBY
  ${SRC_DIR}/Plugins/Verby/verby.png
  ${SRC_DIR}/Plugins/Verby/verby_copy.png
  ${SRC_DIR}/Plugins/Verby/verby_opencontainer.png
  ${SRC_DIR}/Plugins/Verby/verby_properties.png
  ${SRC_DIR}/Plugins/Verby/verby_run.png)

add_custom_command (TARGET Plugin.Verby POST_BUILD
  COMMAND ${CMAKE_COMMAND} -E copy_if_different
  ${RESOURCES_VERBY} ${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/$<CONFIG>/plugins/Verby)

##################################################
# python plugins: copy them after the C++ targets
#
# A python plugin is plain data -- nothing links it -- so nothing makes CMake
# copy it after the compiled code. It used to be an ALL target that raced the
# native plugin folders, and an incremental build could still drop the step.
# Pin PythonPlugins.Deploy to the very end of `all`, so plugins/<Name>/ only
# exists once Launchy.exe, PluginInterface, PluginPy, every native plugin and the
# generated .qm files are all in place.
if (WIN32)
  set (PYTHON_PLUGINS_DEPENDS
    Launchy
    Launchy.Translations
    PluginPy
    Plugin.Runner
    Plugin.Tasky
    Plugin.UWPApp
    Plugin.Verby)
else ()
  set (PYTHON_PLUGINS_DEPENDS
    Launchy
    Launchy.Translations
    PluginPy
    Plugin.Runner
    Plugin.Verby)
endif ()

add_dependencies (PythonPlugins.Deploy ${PYTHON_PLUGINS_DEPENDS})

##################################################
# Windows utilities
#
# dist/win/utilities/ holds ready made shortcut and script collections
# (SpecialFolders/, SystemPower/, UsefulScripts/) that a manual deploy drops
# next to the executable -- Launchy indexes them as ordinary files, so they
# have to sit in the run tree. Nothing links them, so no dependency would ever
# pull them into the build: this is an ALL target that copies the whole folder
# on every build (copy_directory only rewrites files whose contents differ,
# so an incremental build stays cheap).
#
# Same rule as the python plugins below: the source is runtime data, not a
# build input, so files added to it need no CMake change.
if (WIN32)
  get_filename_component (UTILITIES_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../dist/win/utilities ABSOLUTE)
  set (UTILITIES_DEST ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/$<CONFIG>/utilities)

  if (EXISTS "${UTILITIES_DIR}")
    add_custom_target (Utilities.Deploy ALL
      COMMAND ${CMAKE_COMMAND} -E echo
      "copy utilities -> ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/$<CONFIG>/utilities"
      COMMAND ${CMAKE_COMMAND} -E copy_directory
      "${UTILITIES_DIR}" "${UTILITIES_DEST}"
      COMMENT "copy dist/win/utilities into target directory")
  else ()
    message (WARNING "utilities directory not found: ${UTILITIES_DIR}, "
      "the compiled tree will be missing SpecialFolders/SystemPower/UsefulScripts")
  endif ()
endif ()

##################################################
# Unit tests (QTest + CTest)
#
# Tests live in <repo>/test/ and are driven by CTest. They link PluginInterface
# directly to exercise the real code. See test/CMakeLists.txt for how to add
# a test (drop a Test<Name>.cpp + one launchy_add_test line).
#
# Gated behind LAUNCHY_BUILD_TESTS (OFF by default). When OFF, neither
# enable_testing() nor the test subdirectory runs, so the build neither needs
# the Qt5 Test component nor writes any CTest files -- `ctest` then simply has
# nothing to do. Enable with -DLAUNCHY_BUILD_TESTS=ON.
#
# enable_testing() must be called here at the top level (not inside
# test/CMakeLists.txt) so CMake writes the top-level build/CTestTestfile.cmake
# that recurses into test/ -- calling it only inside test/ leaves `ctest` at
# the build root with "No tests were found".
if (LAUNCHY_BUILD_TESTS)
    enable_testing ()
    add_subdirectory (../test ${CMAKE_BINARY_DIR}/test)
endif ()


