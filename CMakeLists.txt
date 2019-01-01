cmake_minimum_required(VERSION 3.12)
project (smaa-nuke)

set(CMAKE_CXX_STANDARD 98)
#set(CMAKE_VERBOSE_MAKEFILE ON)

# Append project Modules directory.
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/resource/cmake")

# Locate Nuke.
find_package(Nuke REQUIRED)

# Convert blink scripts into header files.
include(ConvertBlinkScripts)

# Include Nuke headers.
include_directories(${NUKE_INCLUDE_DIR})

# Include converted blink headers.
include_directories(${BLINK_HEADER_DIR})

# Include Nuke libraries.
link_directories(${NUKE_LIBRARY_DIR})

# Add plugin as shared library.
add_library(Smaa SHARED source/Smaa.cpp ${BLINK_HEADERS})

# Add Nuke DDImage and RIPFramework as targets.
target_link_libraries(Smaa DDImage)
target_link_libraries(Smaa RIPFramework)

# Prevent lib prefix on filesnames.
set_target_properties(Smaa PROPERTIES PREFIX "")

# Deactivate RPATH.
SET(CMAKE_SKIP_RPATH TRUE)

# Once built, copy the library into install location.
install(TARGETS Smaa DESTINATION ${CMAKE_INSTALL_PREFIX}/lib/nuke-${NUKE_VERSION})
