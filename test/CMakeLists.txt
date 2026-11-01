##################################################
# Unit tests (QTest + CTest)
#
# Tests live in <repo>/test/. Each test is a QTest executable that links the
# libraries under test directly, so it exercises the real code rather than a
# copy. QTEST_APPLESS_MAIN is used throughout: the tested code is headless
# (QtCore logic), so no QApplication is needed.
#
# Output layout: test executables are NOT dumped into the app run tree
# (build/bin/<Config>) where Launchy.exe and its Qt DLLs live. Each test lands
# in its own self-contained directory (build/test/<Config>) together with the
# DLLs it needs to run -- PluginInterface.dll plus the Qt5Core/Qt5Test DLLs. This keeps
# the app tree clean (a release archive can `*` it without excluding test
# artifacts) and lets a test run without the app tree on PATH.
#
# CTest integration:
#   enable_testing() + add_test() register each executable with CTest, so
#   `ctest` (or `ctest --test-dir build`) runs them all and reports pass/fail.
#
# Adding a new test:
#   1. drop a Test<Name>.cpp here (it must end by including its own .moc)
#   2. add one line below:  launchy_add_test(TestName)
#   The helper names the executable, sets its output dir, links Qt::Test + the
#   libraries under test, copies the runtime DLLs, and registers the CTest case.

find_package (Qt${QT_MAJOR_VERSION} COMPONENTS Test REQUIRED)

enable_testing ()

# --- helper to declare a QTest executable + its CTest case ---
#
# Any extra arguments are .cpp files from src/Launchy that the test compiles
# into itself. The application is one executable, so a Launchy-side unit can
# only be exercised headless by building it into the test binary:
#
#   launchy_add_test (TestFileSearch ${SRC_DIR}/Launchy/FileSearch.cpp)
function (launchy_add_test test_name)
    add_executable (${test_name} ${test_name}.cpp ${ARGN})

    # Route the test exe (and its .pdb) into build/test/<Config>, away from the
    # app run tree in build/bin/<Config>. CMAKE_RUNTIME_OUTPUT_DIRECTORY is
    # global and would also move the app targets, so this is set per-target.
    set_target_properties (${test_name} PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/test/$<CONFIG>")

    # Automoc is on globally (AUTOMOC), but Qt's default macro includes the
    # .moc directly, so it must exist before compile. AUTOMOC handles the
    # Q_OBJECT header case; for the "include the .moc" pattern Qt ships, the
    # .moc is produced automatically too. Nothing to do here beyond AUTOMOC.
    target_include_directories (${test_name}
        PRIVATE ${SRC_DIR}
        PRIVATE ${DEPS_DIR})

    # A test carrying a Launchy-side unit also needs that unit's own headers and
    # the platform seam, which the app side asks directly (FileSearch looks up
    # the network neighbourhood). The platform library is Qt Core only, so this
    # keeps the test headless.
    if (ARGN)
        target_include_directories (${test_name} PRIVATE ${SRC_DIR}/Launchy)
        target_link_libraries (${test_name} PRIVATE LaunchyPlatform)
    endif ()

    target_link_libraries (${test_name}
        PRIVATE Qt${QT_MAJOR_VERSION}::Test
        PRIVATE LaunchyCore
        PRIVATE PluginInterface)

    # A test links PluginInterface (SHARED -> PluginInterface.dll) and Qt5::Test.
    # Neither DLL sits next to the test exe (PluginInterface.dll is in the app
    # tree, Qt5Test.dll is only deployed by windeployqt into the app tree once).
    # Copy the three runtime DLLs the test needs -- PluginInterface.dll,
    # Qt5Core.dll, Qt5Test.dll -- into the test's own directory so it is
    # self-contained and does not depend on the app tree or PATH.
    if (WIN32)
        # Qt5Test_DIR is <prefix>/lib/cmake/Qt5Test, so ../../.. is <prefix>.
        get_filename_component (_qt_bin "${Qt5Test_DIR}/../../.." ABSOLUTE)
        set (_qt_bin "${_qt_bin}/bin")

        set (_qt_test_dll   "${_qt_bin}/Qt5Test$<$<CONFIG:Debug>:d>.dll")
        set (_qt_core_dll   "${_qt_bin}/Qt5Core$<$<CONFIG:Debug>:d>.dll")

        add_custom_command (TARGET ${test_name} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
                "$<TARGET_FILE:PluginInterface>" "$<TARGET_FILE_DIR:${test_name}>"
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
                "${_qt_core_dll}" "$<TARGET_FILE_DIR:${test_name}>"
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
                "${_qt_test_dll}" "$<TARGET_FILE_DIR:${test_name}>"
            COMMENT "copy PluginInterface.dll + Qt5Core.dll + Qt5Test.dll next to the test")
    endif ()

    add_test (NAME ${test_name} COMMAND ${test_name})
endfunction ()

##################################################
# Tests
launchy_add_test (TestCatalogItem)
launchy_add_test (TestCatalogSearch)
launchy_add_test (TestCatalogFast)
launchy_add_test (TestCatalogSlow)
launchy_add_test (TestInputDataList)
launchy_add_test (TestCommandHistory)
launchy_add_test (TestSettingsManager)
launchy_add_test (TestPluginRegistry)
launchy_add_test (TestPlatform)
# The one test that exercises the seam itself: platform() and the inert fallback
# both belong to the platform library, so this test links it.
target_link_libraries (TestPlatform PRIVATE LaunchyPlatform)
launchy_add_test (TestOptionItem)
# Compile-time only: the test asserts the stored proxy type against the
# QNetworkProxy enumerator, and an enumerator leaves no symbol behind -- the
# binary does not import Qt5Network.dll (same reasoning as LaunchyCore).
target_link_libraries (TestOptionItem PRIVATE Qt${QT_MAJOR_VERSION}::Network)
launchy_add_test (TestVersionTag)
launchy_add_test (TestInputData)
launchy_add_test (TestPluginInfo)
launchy_add_test (TestRuntime)
launchy_add_test (TestUnicodeTable)

# Application units: no library holds them, so the test compiles the .cpp it
# needs. FileSearch is the only search stage outside the catalogs; Logger and
# MemProfiler are the two process-wide facilities every other file calls.
launchy_add_test (TestFileSearch ${SRC_DIR}/Launchy/FileSearch.cpp)
launchy_add_test (TestLogger ${SRC_DIR}/Launchy/Logger.cpp)
launchy_add_test (TestMemProfiler ${SRC_DIR}/Launchy/MemProfiler.cpp)

##################################################
# PluginPy: tested from python, not from C++
#
# launchy.pyd IS a python extension module -- its whole surface (the CatItem and
# Plugin bindings, launchy_util) only exists inside an interpreter, so no C++
# test can reach it. The interpreter is the one cmake found, which is the same
# 3.6.8 the .pyd is built against (src/CMakeLists.txt enforces it), and it is
# pointed at the application tree: that is where launchy.pyd, the Qt DLLs it
# links against and the deployed plugins live.
add_test (NAME TestPluginPy
    COMMAND ${Python_EXECUTABLE}
            ${CMAKE_CURRENT_SOURCE_DIR}/TestPluginPy.py
            $<TARGET_FILE_DIR:Launchy>)

# The application tree only becomes runnable after windeployqt (CI runs ctest
# before that), so a missing Qt runtime there is an unprepared tree, not a
# failing test. The script exits 77 for that and ctest reports it as skipped.
set_tests_properties (TestPluginPy PROPERTIES SKIP_RETURN_CODE 77)

##################################################
# PluginPy: the C++ side of a python plugin
#
# PluginMgr, PluginLoader and PluginWrapper::msg only exist with an interpreter
# in the process, so this test links PluginPy and carries its own copy of the
# runtime -- the same self-contained layout as the other tests: python36.dll,
# python/launchy.pyd, python/launchy_util.py and the Qt DLLs the .pyd links
# against land next to the test executable.
launchy_add_test (TestPluginPyLoader)
target_link_libraries (TestPluginPyLoader PRIVATE PluginPy)
target_include_directories (TestPluginPyLoader PRIVATE ${SRC_DIR}/PluginPy)

get_filename_component (_qt_libexec "${Qt5Test_DIR}/../../.." ABSOLUTE)
set (_qt_test_bin "${_qt_libexec}/bin")
if (NOT EXISTS "${Python_ROOT_DIR}/python36.dll")
    message (FATAL_ERROR "python36.dll not found in ${Python_ROOT_DIR}; pass "
        "-DPython_ROOT_DIR=<3.6.8 install> (see the Python section of "
        "src/CMakeLists.txt)")
endif ()
target_compile_definitions (TestPluginPyLoader PRIVATE
    "LAUNCHY_TEST_PYTHON_HOME=\"${Python_ROOT_DIR}\"")

add_custom_command (TARGET TestPluginPyLoader POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory
        "$<TARGET_FILE_DIR:TestPluginPyLoader>/python"
    # The module and its helper: PluginMgr puts <exe dir>/python on sys.path
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "$<TARGET_FILE:PluginPy>" "$<TARGET_FILE_DIR:TestPluginPyLoader>/python/launchy.pyd"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${SRC_DIR}/PluginPy/launchy_util.py" "$<TARGET_FILE_DIR:TestPluginPyLoader>/python"
    # PluginPy.dll itself, the Qt DLLs it imports and the interpreter
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "$<TARGET_FILE:PluginPy>" "$<TARGET_FILE_DIR:TestPluginPyLoader>"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${_qt_test_bin}/Qt5Gui$<$<CONFIG:Debug>:d>.dll" "$<TARGET_FILE_DIR:TestPluginPyLoader>"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${_qt_test_bin}/Qt5Widgets$<$<CONFIG:Debug>:d>.dll" "$<TARGET_FILE_DIR:TestPluginPyLoader>"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${Python_ROOT_DIR}/python36.dll" "$<TARGET_FILE_DIR:TestPluginPyLoader>"
    COMMENT "copy PluginPy + python36.dll + Qt GUI DLLs next to the test")
