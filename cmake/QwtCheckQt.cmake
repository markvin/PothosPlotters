########################################################################
# Check that a Qwt shared library is built for Qt 6 (cmake -P script).
# FindQwt.cmake runs it when pkg-config does not give the Qt version of
# Qwt. A Qwt for Qt 5 loads Qt 5 next to Qt 6 and crashes at run time.
#
# Required: -DQWT_LIBRARY=<the Qwt shared library: .so, .dylib, .dll or
#           a .framework directory>
# The script fails (exit code not 0) when a dependency of the library is a
# Qt 5 library (libQt5*, Qt5*.dll, Qt*.framework/Versions/5), or when no
# dependency is a Qt 6 library.
########################################################################
cmake_minimum_required(VERSION 3.25) #policies for cmake -P scripts
if (NOT QWT_LIBRARY)
    message(FATAL_ERROR "QwtCheckQt: QWT_LIBRARY is not set")
endif ()
if (NOT EXISTS "${QWT_LIBRARY}")
    message(FATAL_ERROR "QwtCheckQt: ${QWT_LIBRARY} does not exist")
endif ()

#a Qwt framework is a directory: examine the library in it
if (IS_DIRECTORY "${QWT_LIBRARY}")
    get_filename_component(name "${QWT_LIBRARY}" NAME_WE)
    set(QWT_LIBRARY "${QWT_LIBRARY}/${name}")
endif ()

#only the names of the dependencies count: a name that resolves to two
#paths (CONFLICTING_DEPENDENCIES_PREFIX) must not stop the check
file(GET_RUNTIME_DEPENDENCIES
    LIBRARIES "${QWT_LIBRARY}"
    RESOLVED_DEPENDENCIES_VAR resolved
    UNRESOLVED_DEPENDENCIES_VAR unresolved
    CONFLICTING_DEPENDENCIES_PREFIX conflicting)
foreach(name IN LISTS conflicting_FILENAMES)
    list(APPEND resolved ${conflicting_${name}})
endforeach()

set(qt5 "")
set(qt6 FALSE)
foreach(dep IN LISTS resolved unresolved)
    get_filename_component(dep_name "${dep}" NAME)
    if (dep_name MATCHES "^(lib)?Qt5" OR dep MATCHES "Qt[A-Za-z]+\\.framework/Versions/5/")
        list(APPEND qt5 "${dep}")
    elseif (dep_name MATCHES "^(lib)?Qt6" OR dep MATCHES "Qt[A-Za-z]+\\.framework/Versions/A/")
        set(qt6 TRUE)
    endif ()
endforeach()

if (qt5)
    list(JOIN qt5 ", " qt5_text)
    message(FATAL_ERROR "QwtCheckQt: ${QWT_LIBRARY} is built for Qt 5 (it uses ${qt5_text}). "
        "PothosPlotters needs a Qwt for Qt 6.")
endif ()
if (NOT qt6)
    message(FATAL_ERROR "QwtCheckQt: ${QWT_LIBRARY} uses no Qt 6 library. "
        "PothosPlotters needs a Qwt for Qt 6.")
endif ()
message(STATUS "QwtCheckQt: ${QWT_LIBRARY} is built for Qt 6")
