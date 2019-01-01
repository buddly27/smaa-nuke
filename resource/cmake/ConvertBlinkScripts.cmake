# ============================================================================
#
# Copyright (C) 2019, Jeremy Retailleau
#
# This source code is licensed under the MIT license found in the
# LICENSE file in the root directory of this source tree.
#
# Convert all Blink Scripts into header files.
#
# Variables defined by this module:
#     BLINK_HEADERS
#     BLINK_HEADER_DIR
#
# Usage:
#     INCLUDE(ConvertBlinkScripts)
#
# ============================================================================

add_executable(
    blink_to_header "${CMAKE_SOURCE_DIR}/resource/tools/blink_to_header.cpp"
)

file(GLOB _blink_FILES "${CMAKE_SOURCE_DIR}/resource/blink/*.blk")

set(BLINK_HEADERS)
set(BLINK_HEADER_DIR "${CMAKE_BINARY_DIR}/include")

# Create include folder.
file(MAKE_DIRECTORY "${BLINK_HEADER_DIR}")

foreach(_blink_SOURCE ${_blink_FILES})
    get_filename_component(_blink_NAME "${_blink_SOURCE}" NAME)
    string(REGEX REPLACE ".blk" "" _blink_VAR_NAME ${_blink_NAME})
    string(REGEX REPLACE ".blk" ".h" _blink_HEADER ${_blink_NAME})
    set(_blink_TARGET "${BLINK_HEADER_DIR}/${_blink_HEADER}")

    add_custom_command(
        OUTPUT ${_blink_HEADER}
        PRE_BUILD COMMAND blink_to_header ${_blink_SOURCE} ${_blink_TARGET}
        DEPENDS ${_blink_SOURCE}
        WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
        COMMENT "Convert ${_blink_NAME} to ${_blink_HEADER}"
    )

    # Placeholders to get coherent pre-build linting in IDE.
    if (NOT EXISTS ${_blink_TARGET})
        file(WRITE ${_blink_TARGET} "const char ${_blink_VAR_NAME} [] = \"\";")
    endif()

    list(APPEND BLINK_HEADERS "${_blink_HEADER}")
endforeach()

add_custom_target(blink_headers ALL DEPENDS "${BLINK_HEADERS}")
