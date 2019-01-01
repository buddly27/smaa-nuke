# ============================================================================
#
# Copyright (C) 2019, Jeremy Retailleau
#
# This source code is licensed under the MIT license found in the
# LICENSE file in the root directory of this source tree.
#
# Variables defined by this module:
#     NUKE_PATH
#     NUKE_VERSION_MAJOR
#     NUKE_VERSION_MINOR
#     NUKE_VERSION_PATCH
#     NUKE_VERSION
#     NUKE_INCLUDE_DIR
#     NUKE_LIBRARY_DIR
#
# Usage:
#     FIND_PACKAGE(Nuke)
#     FIND_PACKAGE(Nuke REQUIRED)
#
# Note:
#     You can tell the module where Nuke is installed by setting
#     the NUKE_PATH or NUKE_LOCATION environment variables.
#
# ============================================================================

# Look for Nuke on the file system if necessary.
if(NOT DEFINED NUKE_PATH)
    # Define a sensible Nuke location path default value if necessary.
    if(NOT DEFINED NUKE_LOCATION)
        if(WIN32)
            set(NUKE_LOCATION "C:/Program Files")
        elseif(APPLE)
            set(NUKE_LOCATION "/Applications")
        else()
            set(NUKE_LOCATION "/usr/local")
        endif()
    endif()

    # Search Nuke's highest version on the file system.
    file(GLOB _nuke_FOLDERS "${NUKE_LOCATION}/Nuke*")

    # Get the highest version if possible.
    if(_nuke_FOLDERS)
        list(SORT _nuke_FOLDERS)
        list(REVERSE _nuke_FOLDERS)
        list(LENGTH _nuke_FOLDERS _nuke_FOLDERS_LENGTH)
        if(${_nuke_FOLDERS_LENGTH} GREATER 0)
            list(GET _nuke_FOLDERS 0 NUKE_PATH)
        endif()
    endif()

    # Raise error if Nuke not found.
    if(NOT DEFINED NUKE_PATH)
        message(FATAL_ERROR "Impossible to find Nuke in ${NUKE_LOCATION}")
    endif()
endif()

# Inform the user on the Nuke path being used.
message("Nuke Path: ${NUKE_PATH}")

# Extract versions from Nuke root folder.
string(
    REGEX REPLACE "([0-9]+)\\.([0-9]+)\\v([0-9]+)" "\\0;\\1;\\2;\\3"
    _nuke_VERSION_LIST ${NUKE_PATH}
)
list(LENGTH _nuke_VERSION_LIST _list_LENGTH)
if(_list_LENGTH GREATER 2)
    list(GET _nuke_VERSION_LIST 1 NUKE_VERSION_MAJOR)
    list(GET _nuke_VERSION_LIST 2 NUKE_VERSION_MINOR)
    list(GET _nuke_VERSION_LIST 3 NUKE_VERSION_PATCH)
    set(
        NUKE_VERSION
        "${NUKE_VERSION_MAJOR}.${NUKE_VERSION_MINOR}v${NUKE_VERSION_PATCH}"
    )
    message("Nuke Version: ${NUKE_VERSION}")
else()
    set(NUKE_VERSION_MAJOR "undefined")
    set(NUKE_VERSION_MINOR "undefined")
    set(NUKE_VERSION_PATCH "undefined")
    set(NUKE_VERSION "undefined")
    message(
        WARNING "Impossible to deduce Nuke versions from folder ${NUKE_PATH}"
    )
endif()

# Compute NUKE_INCLUDE_DIR and NUKE_LIBRARY_DIR variables.
if(APPLE)
    get_filename_component(_nuke_ROOT_FOLDER ${NUKE_PATH} NAME)
    set(
        NUKE_INCLUDE_DIR
        "${NUKE_PATH}/${_nuke_ROOT_FOLDER}.app/Contents/MacOS/include"
    )
    set(
        NUKE_LIBRARY_DIR
        "${NUKE_PATH}/${_nuke_ROOT_FOLDER}.app/Contents/MacOS"
    )
else()
    set(NUKE_INCLUDE_DIR "${NUKE_PATH}/include")
    set(NUKE_LIBRARY_DIR "${NUKE_PATH}")
endif()

# Ensure that folders exist.
if(NOT EXISTS "${NUKE_INCLUDE_DIR}")
    message(FATAL_ERROR "Path does not exist: ${NUKE_INCLUDE_DIR}")
endif()

if(NOT EXISTS "${NUKE_LIBRARY_DIR}")
    message(FATAL_ERROR "Path does not exist: ${NUKE_LIBRARY_DIR}")
endif()
