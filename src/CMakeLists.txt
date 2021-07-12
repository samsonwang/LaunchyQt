
cmake_minimum_required (VERSION 3.14.5)

project ("Launchy")

if (MSVC)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
else ()
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ~/Launchy)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ~/Launchy)
endif ()

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

add_subdirectory ("Launchy")
add_subdirectory ("LaunchyLib")
add_subdirectory ("PluginPy")

#add_subdirectory ("Plugins/Util")
