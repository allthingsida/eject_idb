cmake_minimum_required(VERSION 3.27)

project(eject_idb)

set(CMAKE_CXX_STANDARD 17)

# Include IDA SDK bootstrap
include($ENV{IDASDK}/src/cmake/bootstrap.cmake)
find_package(idasdk REQUIRED)

# Standalone executable (not an IDA addon)
add_executable(eject_idb eject_idb.cpp utils.hpp README.md)
set_source_files_properties(README.md PROPERTIES HEADER_FILE_ONLY TRUE)

# Deploy executable to IDA bin directory
install(TARGETS eject_idb
    RUNTIME DESTINATION "${IDABIN}"
)

# IDA plugin (automatically deployed to $IDABIN/plugins/)
ida_add_plugin(eject_plugin
    SOURCES
        eject_plugin.cpp
        utils.hpp
        README.md
    METADATA_JSON
        ida-plugin.json
    DEBUG_ARGS
        "-t"
        "-z10000"
)
set_source_files_properties(README.md PROPERTIES HEADER_FILE_ONLY TRUE)
