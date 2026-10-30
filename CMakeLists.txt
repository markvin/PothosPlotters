########################################################################
# PothosPlotters: plotter widgets for the Pothos GUI.
# Builds against an installed Pothos core (find_package(Pothos)), Qt 6
# and Qwt 6.3 for Qt 6. The modules use only the public API of the core.
# Qt and Qwt come from find_package: from the system, or from an install
# that the user gives with CMAKE_PREFIX_PATH (or Qt6_DIR, Qwt_ROOT).
# At run time the plotters need PothosComms (/comms/wave_trigger).
########################################################################
cmake_minimum_required(VERSION 3.25)
project(PothosPlotters CXX)

option(PLOTTERS_BUILD_TESTS "Register the CTest tests of the plotters" ${PROJECT_IS_TOP_LEVEL})
option(PLOTTERS_WARNINGS_AS_ERRORS "Treat compiler warnings as errors (GCC and Clang)" OFF)

#the core defines the variable POTHOS_ROOT (its install prefix), and the
#runtime environment often sets POTHOS_ROOT too. find_package must not use
#them as search prefixes (CMP0144): it could find another core than
#CMAKE_PREFIX_PATH gives. NO_PACKAGE_ROOT_PATH keeps them out.
if(POLICY CMP0144)
    cmake_policy(SET CMP0144 NEW)
endif()

find_package(Pothos CONFIG REQUIRED NO_PACKAGE_ROOT_PATH)

include(GNUInstallDirs)

#the core loads modules from its own prefix (POTHOS_ROOT): install there
#when the user does not give CMAKE_INSTALL_PREFIX
if(CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)
    set(CMAKE_INSTALL_PREFIX "${POTHOS_ROOT}" CACHE PATH "Install prefix (default: the prefix of the Pothos core)" FORCE)
    message(STATUS "Install prefix: ${CMAKE_INSTALL_PREFIX} (the prefix of the Pothos core)")
endif()

########################################################################
# Qt 6
########################################################################
find_package(Qt6 6.5 CONFIG COMPONENTS Widgets)
if(NOT Qt6_FOUND)
    #Qt can be present without one of its own dependencies (for example the
    #OpenGL headers for Qt Gui): show the reason that Qt gives
    set(plotters_qt_reason "")
    if(Qt6_NOT_FOUND_MESSAGE)
        set(plotters_qt_reason "Reason given by Qt: ${Qt6_NOT_FOUND_MESSAGE}\n")
    endif()
    message(FATAL_ERROR
        "PothosPlotters: Qt 6 (>= 6.5) with the Widgets component is missing.\n"
        "${plotters_qt_reason}"
        "Install the Qt 6 package of the system (for example qt6-base; on Debian "
        "and Ubuntu also the OpenGL headers, for example libgl-dev), "
        "or give CMAKE_PREFIX_PATH or Qt6_DIR. Refer to README.md.")
endif()
message(STATUS "PothosPlotters: Qt ${Qt6_VERSION}")

########################################################################
# Qwt 6.3 for Qt 6 (cmake/FindQwt.cmake)
########################################################################
list(APPEND CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/cmake)
find_package(Qwt 6.3.0)
if(NOT Qwt_FOUND)
    message(FATAL_ERROR
        "PothosPlotters: Qwt (>= 6.3.0) for Qt 6 is missing.\n"
        "Install the Qwt package for Qt 6 of the system (for example qwt on Arch), "
        "or give CMAKE_PREFIX_PATH or Qwt_ROOT to a Qwt for Qt 6. Refer to README.md.")
endif()
message(STATUS "PothosPlotters: Qwt ${Qwt_VERSION} (${Qwt_LIBRARY})")

########################################################################
# Common settings of the targets
########################################################################
#plotters_warnings(<target>): the common warnings (GCC and Clang, as the core),
#errors with PLOTTERS_WARNINGS_AS_ERRORS (CI)
function(plotters_warnings target)
    if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
        target_compile_options(${target} PRIVATE -Wall -Wextra)
        if(PLOTTERS_WARNINGS_AS_ERRORS)
            target_compile_options(${target} PRIVATE -Werror)
        endif()
    endif()
endfunction()

#a Qt or Qwt outside the system dirs (an install of the user or of CI):
#the installed libraries must find them at run time. On Windows the Qt and
#Qwt DLL dirs are on PATH.
set(PLOTTERS_EXTRA_RPATH "")
get_filename_component(plotters_qwt_libdir "${Qwt_LIBRARY}" DIRECTORY)
foreach(dir "${QT6_INSTALL_PREFIX}/${QT6_INSTALL_LIBS}" "${plotters_qwt_libdir}")
    if(NOT dir IN_LIST CMAKE_CXX_IMPLICIT_LINK_DIRECTORIES AND NOT dir IN_LIST PLOTTERS_EXTRA_RPATH)
        list(APPEND PLOTTERS_EXTRA_RPATH "${dir}")
    endif()
endforeach()
if(PLOTTERS_EXTRA_RPATH)
    message(STATUS "PothosPlotters: install RPATH ${PLOTTERS_EXTRA_RPATH}")
endif()

#plotters_target(<target> [MODULE]): C++23, moc, warnings and the install
#RPATH. A module is installed in <libdir>/Pothos/modules<ABI>/plotters and
#finds PothosPlotterUtils in <libdir> with a relative RPATH: the tests load
#the modules from a stage prefix, an absolute path goes to another prefix.
function(plotters_target target)
    cmake_parse_arguments(arg "MODULE" "" "" ${ARGN})
    set_target_properties(${target} PROPERTIES
        AUTOMOC ON
        CXX_EXTENSIONS OFF)
    target_compile_features(${target} PRIVATE cxx_std_23)
    plotters_warnings(${target})
    set(rpath ${PLOTTERS_EXTRA_RPATH})
    if(arg_MODULE)
        if(APPLE)
            list(APPEND rpath "@loader_path/../../..")
        elseif(UNIX)
            list(APPEND rpath "$ORIGIN/../../..")
        endif()
    endif()
    if(UNIX AND rpath)
        set_property(TARGET ${target} APPEND PROPERTY INSTALL_RPATH "${rpath}")
    endif()
endfunction()

########################################################################
# Plotter utils library
########################################################################
add_library(PothosPlotterUtils SHARED
    PlotterUtils/PothosPlotPicker.cpp
    PlotterUtils/PothosPlotter.cpp
    PlotterUtils/PothosPlotUtils.cpp
    PlotterUtils/Window.cpp)
target_include_directories(PothosPlotterUtils PUBLIC ${PROJECT_SOURCE_DIR}/PlotterUtils)
set_target_properties(PothosPlotterUtils PROPERTIES
    DEFINE_SYMBOL POTHOS_PLOTTER_UTILS_MAKEDLL
    VERSION 0.3.0)
target_link_libraries(PothosPlotterUtils
    PUBLIC Pothos::Pothos Qt6::Widgets Qwt::Qwt)
plotters_target(PothosPlotterUtils)
install(TARGETS PothosPlotterUtils
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR} # .so file
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR} # .lib file
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR} # .dll file
)

########################################################################
# Modules
########################################################################
add_subdirectory(Constellation)
add_subdirectory(LogicAnalyzer)
add_subdirectory(Periodogram)
add_subdirectory(QwtWidgets)
add_subdirectory(Spectrogram)
add_subdirectory(WaveMonitor)
add_subdirectory(Tests)

########################################################################
# Tests: the modules are staged into a prefix inside the build tree and
# loaded with POTHOS_PLUGIN_PATH by the PothosUtil of the installed core.
# The plotters need no display: Qt uses the offscreen platform.
########################################################################
if(PLOTTERS_BUILD_TESTS)
    enable_testing()
    set(PLOTTERS_STAGE_DIR ${PROJECT_BINARY_DIR}/stage)
    set(PLOTTERS_SELFTEST_TIMEOUT 600 CACHE STRING "Timeout in seconds for the self-tests")

    add_test(NAME plotters.stage
        COMMAND ${CMAKE_COMMAND} --install ${PROJECT_BINARY_DIR} --prefix ${PLOTTERS_STAGE_DIR} --config $<CONFIG>)
    set_tests_properties(plotters.stage PROPERTIES FIXTURES_SETUP plotters_stage)

    get_property(plotters_tests GLOBAL PROPERTY POTHOS_PLOTTERS_TESTS)
    add_test(NAME plotters.selftests
        COMMAND ${CMAKE_COMMAND}
            -DPREFIX=${POTHOS_ROOT}
            -DTEST_HOME_DIR=${PROJECT_BINARY_DIR}/test-home
            -DBINDIR=bin
            -DLIBDIR=${CMAKE_INSTALL_LIBDIR}
            -DPLUGIN_PATH=${PLOTTERS_STAGE_DIR}/${CMAKE_INSTALL_LIBDIR}/Pothos/modules${POTHOS_ABI_VERSION}
            -DSTAGE_BINDIR=${PLOTTERS_STAGE_DIR}/${CMAKE_INSTALL_BINDIR}
            "-DTEST_PATHS=${plotters_tests}"
            -P ${PROJECT_SOURCE_DIR}/cmake/PothosRunSelfTests.cmake)
    set_tests_properties(plotters.selftests PROPERTIES
        FIXTURES_REQUIRED plotters_stage
        ENVIRONMENT QT_QPA_PLATFORM=offscreen
        TIMEOUT ${PLOTTERS_SELFTEST_TIMEOUT})
endif()
