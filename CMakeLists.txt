cmake_minimum_required(VERSION 2.8)

project(cpp)

# Add "deps" to search prefix for convenience.
set(CMAKE_PREFIX_PATH "${CMAKE_BINARY_DIR}/deps" "${CMAKE_SOURCE_DIR}/deps" "${CMAKE_PREFIX_PATH}")
# The libraries have the "lib" prefix even on Windows.
set(CMAKE_FIND_LIBRARY_PREFIXES "lib" "${CMAKE_FIND_LIBRARY_PREFIXES}")

# Set rpath for Mac and Linux.
if(APPLE)
    SET(CMAKE_INSTALL_RPATH "${CMAKE_INSTALL_RPATH}:@executable_path/../lib:@executable_path/" )
else(UNIX)
    SET(CMAKE_INSTALL_RPATH "${CMAKE_INSTALL_RPATH}:$ORIGIN/../lib:$ORIGIN/" )
endif()

# Find libraries
find_library(LIB_EGL    NAMES EGL)
find_library(LIB_GLES   NAMES GLESv2)

if(NOT LIB_EGL)
    message(FATAL_ERROR "EGL library not found")
endif()

if(NOT LIB_GLES)
    message(FATAL_ERROR "GLES library not found")
endif()


add_executable(get_image get_image.cpp lodepng.cpp common.cpp)
add_executable(get_gl_info get_gl_info.cpp common.cpp)

target_link_libraries(get_image ${LIB_EGL} ${LIB_GLES})
target_link_libraries(get_gl_info ${LIB_EGL} ${LIB_GLES})

target_include_directories(get_image PUBLIC include/)
target_include_directories(get_gl_info PUBLIC include/)

install(TARGETS get_image get_gl_info
    DESTINATION bin
)

if(CMAKE_COMPILER_IS_GNUCXX)
    set(CMAKE_CXX_FLAGS "-Wall -std=c++11 ${CMAKE_CXX_FLAGS}")
endif()

