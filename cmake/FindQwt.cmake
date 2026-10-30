########################################################################
# Find Qwt for Qt 6.
#
# Gives the imported target Qwt::Qwt and the variables Qwt_FOUND,
# Qwt_VERSION, Qwt_INCLUDE_DIR and Qwt_LIBRARY. The caller must find Qt 6
# (Qt6::Widgets) first.
#
# find_path/find_library search Qwt_ROOT, CMAKE_PREFIX_PATH and the system
# directories. The pkg-config module Qt6Qwt6 (Arch, a Qwt installed with
# qmake on Linux and macOS) only gives hints: a prefix of the user comes
# first. Thus QwtCheckQt.cmake always examines the libraries that the
# found Qwt library uses, and refuses a Qwt for Qt 5.
########################################################################
include(FindPackageHandleStandardArgs)

set(_qwt_qt5_message "")

find_package(PkgConfig QUIET)
if (PKG_CONFIG_FOUND)
    pkg_check_modules(PC_QWT QUIET Qt6Qwt6)
endif ()

find_path(Qwt_INCLUDE_DIR
    NAMES qwt_global.h
    HINTS ${PC_QWT_INCLUDE_DIRS}
    PATH_SUFFIXES qwt-qt6 qwt6 qwt include include/qwt qwt.framework/Headers lib/qwt.framework/Headers)

find_library(Qwt_LIBRARY
    NAMES qwt-qt6 qwt qwtd
    HINTS ${PC_QWT_LIBRARY_DIRS})

if (Qwt_INCLUDE_DIR AND EXISTS "${Qwt_INCLUDE_DIR}/qwt_global.h")
    file(STRINGS "${Qwt_INCLUDE_DIR}/qwt_global.h" _qwt_version_line
        REGEX "define[ \t]+QWT_VERSION_STR[ \t]+\"")
    string(REGEX REPLACE ".*QWT_VERSION_STR[ \t]+\"([^\"]+)\".*" "\\1"
        Qwt_VERSION "${_qwt_version_line}")
endif ()

#examine the libraries that the found Qwt library uses (on Windows the
#DLL, not the import library)
set(Qwt_FOR_QT6 TRUE)
if (Qwt_LIBRARY)
    set(_qwt_check_file "${Qwt_LIBRARY}")
    if (WIN32)
        get_filename_component(_qwt_dir "${Qwt_LIBRARY}" DIRECTORY)
        get_filename_component(_qwt_name "${Qwt_LIBRARY}" NAME_WE)
        find_file(_qwt_dll NAMES "${_qwt_name}.dll"
            PATHS "${_qwt_dir}" "${_qwt_dir}/../bin" NO_DEFAULT_PATH NO_CACHE)
        set(_qwt_check_file "${_qwt_dll}")
    endif ()
    if (NOT _qwt_check_file)
        set(Qwt_FOR_QT6 FALSE)
        set(_qwt_qt5_message "The Qwt DLL next to ${Qwt_LIBRARY} is missing. ")
    else ()
        execute_process(
            COMMAND "${CMAKE_COMMAND}" "-DQWT_LIBRARY=${_qwt_check_file}"
                -P "${CMAKE_CURRENT_LIST_DIR}/QwtCheckQt.cmake"
            RESULT_VARIABLE _qwt_check_rc
            OUTPUT_VARIABLE _qwt_check_out
            ERROR_VARIABLE _qwt_check_out)
        if (NOT _qwt_check_rc EQUAL 0)
            set(Qwt_FOR_QT6 FALSE)
            string(STRIP "${_qwt_check_out}" _qwt_check_out)
            set(_qwt_qt5_message "${_qwt_check_out}\n")
        endif ()
    endif ()
endif ()
find_package_handle_standard_args(Qwt
    REQUIRED_VARS Qwt_LIBRARY Qwt_INCLUDE_DIR Qwt_FOR_QT6
    VERSION_VAR Qwt_VERSION
    REASON_FAILURE_MESSAGE "${_qwt_qt5_message}")

if (Qwt_FOUND AND NOT TARGET Qwt::Qwt)
    add_library(Qwt::Qwt UNKNOWN IMPORTED)
    set(_qwt_location "${Qwt_LIBRARY}")
    #a framework: link the library in it
    if (IS_DIRECTORY "${Qwt_LIBRARY}")
        get_filename_component(_qwt_name "${Qwt_LIBRARY}" NAME_WE)
        set(_qwt_location "${Qwt_LIBRARY}/${_qwt_name}")
    endif ()
    set_target_properties(Qwt::Qwt PROPERTIES
        IMPORTED_LOCATION "${_qwt_location}"
        INTERFACE_INCLUDE_DIRECTORIES "${Qwt_INCLUDE_DIR}"
        INTERFACE_LINK_LIBRARIES Qt6::Widgets)
    if (WIN32)
        set_property(TARGET Qwt::Qwt APPEND PROPERTY INTERFACE_COMPILE_DEFINITIONS QWT_DLL)
    endif ()
endif ()

mark_as_advanced(Qwt_INCLUDE_DIR Qwt_LIBRARY)
