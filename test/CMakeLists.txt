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
# DLLs it needs to run -- Launchy.dll plus the Qt5Core/Qt5Test DLLs. This keeps
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
function (launchy_add_test test_name)
    add_executable (${test_name} ${test_name}.cpp)

    # The main CMakeLists sets CMAKE_WIN32_EXECUTABLE globally (the launcher is
    # a GUI app), which would silently mark every test executable as a GUI
    # subsystem binary: no console, no stdout, and CTest could not read a
    # result. QTest must run as a console app, so force that here.
    set_target_properties (${test_name} PROPERTIES WIN32_EXECUTABLE FALSE)

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

    target_link_libraries (${test_name}
        PRIVATE Qt${QT_MAJOR_VERSION}::Test
        PRIVATE LaunchyLib)

    # A test links LaunchyLib (SHARED -> Launchy.dll) and Qt5::Test. Neither
    # DLL sits next to the test exe (Launchy.dll is in the app tree, Qt5Test.dll
    # is only deployed by windeployqt into the app tree once). Copy the three
    # runtime DLLs the test needs -- Launchy.dll, Qt5Core.dll, Qt5Test.dll --
    # into the test's own directory so it is self-contained and does not depend
    # on the app tree or PATH.
    if (WIN32)
        # Qt5Test_DIR is <prefix>/lib/cmake/Qt5Test, so ../../.. is <prefix>.
        get_filename_component (_qt_bin "${Qt5Test_DIR}/../../.." ABSOLUTE)
        set (_qt_bin "${_qt_bin}/bin")

        set (_qt_test_dll   "${_qt_bin}/Qt5Test$<$<CONFIG:Debug>:d>.dll")
        set (_qt_core_dll   "${_qt_bin}/Qt5Core$<$<CONFIG:Debug>:d>.dll")

        add_custom_command (TARGET ${test_name} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
                "$<TARGET_FILE:LaunchyLib>" "$<TARGET_FILE_DIR:${test_name}>"
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
                "${_qt_core_dll}" "$<TARGET_FILE_DIR:${test_name}>"
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
                "${_qt_test_dll}" "$<TARGET_FILE_DIR:${test_name}>"
            COMMENT "copy Launchy.dll + Qt5Core.dll + Qt5Test.dll next to the test")
    endif ()

    add_test (NAME ${test_name} COMMAND ${test_name})
endfunction ()

##################################################
# Tests
launchy_add_test (TestCatalogItem)
