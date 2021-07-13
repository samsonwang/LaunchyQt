# ---------------------------------------------------
# Qt
# ---------------------------------------------------

# add CMAKE_PREFIX_PATH = <qt installed dir> in system PATH
find_package (Qt5 COMPONENTS Widgets WinExtras Network REQUIRED)

set (CMAKE_AUTOUIC ON)
set (CMAKE_AUTOMOC ON)
set (CMAKE_AUTORCC ON)

# source group for qt moc files
set_property (GLOBAL PROPERTY AUTOUIC_SOURCE_GROUP "Generated Files")
set_property (GLOBAL PROPERTY AUTOMOC_SOURCE_GROUP "Generated Files")
set_property (GLOBAL PROPERTY AUTORCC_SOURCE_GROUP "Generated Files")

# qt source files
file (GLOB SOURCES_QT
  *.ui
  *.qrc
  *.ts)
source_group ("Qt Files" FILES ${SOURCES_QT})
list (APPEND SOURCES ${SOURCES_QT})


# link qtmain.lib
if (POLICY CMP0020)
  cmake_policy (SET CMP0020 NEW)
endif ()

# hide console in release
if (WIN32)
  set (CMAKE_WIN32_EXECUTABLE TRUE)
endif ()

if (MSVC)
  set (CMAKE_EXE_LINKER_FLAGS_DEBUG "/SUBSYSTEM:CONSOLE")
  set (CMAKE_EXE_LINKER_FLAGS_RELEASE "/SUBSYSTEM:WINDOWS")
endif ()
