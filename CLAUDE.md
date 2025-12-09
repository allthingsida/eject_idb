# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Build Commands

```bash
# Configure and build
cmake -B build
cmake --build build --config RelWithDebInfo

# Deploy to IDA installation
cmake --install build --config RelWithDebInfo
```

Prerequisites: CMake 3.27+, IDA SDK 9.2+ (set `IDASDK` environment variable), ida-cmake installed, Visual Studio 2022 (Windows) / GCC/Clang (Linux/macOS).

## Architecture

This project is an IDA Pro plugin for emergency IDB saving when IDA hangs or crashes. It consists of two components:

1. **eject_plugin** (`eject_plugin.cpp`) - IDA plugin that:
   - Creates a named semaphore based on the IDB path hash (using djb2)
   - Spawns a background thread waiting on that semaphore
   - When signaled, saves the database as `<name>.ejected.<ext>` and optionally terminates IDA (Windows)
   - Has test modes (arg=0/1) to simulate UI hangs for testing

2. **eject_idb** (`eject_idb.cpp`) - Standalone CLI tool that:
   - Takes an IDB path as argument
   - Opens the corresponding named semaphore and signals it
   - This triggers the plugin to save the database

The two components communicate via a named semaphore (`ejectidb_<hash>`) derived from the full IDB path.

## Key Files

- `utils.hpp` - Shared utilities: `djb2()` hash function, `make_semaphore_name()` for cross-platform semaphore naming
- `ida-plugin.json` - IDA 9.x plugin metadata

## ida-cmake

This project uses ida-cmake for building. The `ida-cmake` agent is available for troubleshooting build issues or creating new IDA addons.
