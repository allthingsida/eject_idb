# Copyright (c) 2019-2026 Elias Bachaalany
# SPDX-License-Identifier: LicenseRef-Human-Origin-Source-1.0
#
# This file is licensed under the Human-Origin Source License v1.0.
# See LICENSE.

cmake_minimum_required(VERSION 3.27)

project(eject_idb)

set(CMAKE_CXX_STANDARD 20)

# Include IDA SDK bootstrap
include($ENV{IDASDK}/ida-cmake/bootstrap.cmake)
find_package(idasdk REQUIRED)

# Resolve libidacpp: prefer the local sibling for development; fall back to
# FetchContent for CI / out-of-tree builds.
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/../libidacpp/CMakeLists.txt")
    add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/../libidacpp ${CMAKE_CURRENT_BINARY_DIR}/libidacpp)
    set(LIBIDACPP_INCLUDE ${CMAKE_CURRENT_SOURCE_DIR}/../libidacpp/include)
else()
    include(FetchContent)
    FetchContent_Declare(
        libidacpp
        GIT_REPOSITORY https://github.com/allthingsida/libidacpp.git
        GIT_TAG main
    )
    FetchContent_MakeAvailable(libidacpp)
    set(LIBIDACPP_INCLUDE ${libidacpp_SOURCE_DIR}/include)
endif()

# Standalone signaler executable (not an IDA addon). It must stay free of IDA
# linkage, so it consumes libidacpp headers directly instead of the
# libidacpp::libidacpp target (which links idasdk when the SDK is present).
add_executable(eject_idb eject_idb.cpp eject_core.hpp README.md)
set_source_files_properties(README.md PROPERTIES HEADER_FILE_ONLY TRUE)
target_include_directories(eject_idb PRIVATE ${LIBIDACPP_INCLUDE})
target_compile_features(eject_idb PRIVATE cxx_std_20)
if(NOT WIN32)
    find_package(Threads REQUIRED)
    target_link_libraries(eject_idb PRIVATE Threads::Threads $<$<PLATFORM_ID:Linux>:rt>)
endif()

# Deploy executable to IDA bin directory at build time
set_target_properties(eject_idb PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${IDABIN}"
)
foreach(config Debug Release RelWithDebInfo MinSizeRel)
    string(TOUPPER ${config} config_upper)
    set_target_properties(eject_idb PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY_${config_upper} "${IDABIN}"
    )
endforeach()

# IDA plugin (automatically deployed to $IDABIN/plugins/)
ida_add_plugin(eject_plugin
    SOURCES
        eject_plugin.cpp
        eject_core.hpp
        utils.hpp
        README.md
    METADATA_JSON
        ida-plugin.json
    DEBUG_ARGS
        "-t"
        "-z10000"
)
set_source_files_properties(README.md PROPERTIES HEADER_FILE_ONLY TRUE)
target_link_libraries(eject_plugin PRIVATE libidacpp::libidacpp)
