#!/bin/bash
set -x
set -e
set -u


CMAKE_BUILD_TYPE="${Configuration}"
INSTALL_DIR="${PROFILE}-${Platform}-${CMAKE_BUILD_TYPE}"
BUILD_DIR="$INSTALL_DIR-build"

mkdir -p "${BUILD_DIR}"
cd "${BUILD_DIR}"
cmake -G "${CMAKE_GENERATOR}" .. "-DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE}" ${CMAKE_OPTIONS}
cmake --build . --config "${CMAKE_BUILD_TYPE}"
cmake "-DCMAKE_INSTALL_PREFIX=../${INSTALL_DIR}" "-DBUILD_TYPE=${CMAKE_BUILD_TYPE}" -P cmake_install.cmake
cd ..

