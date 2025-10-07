# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

Always compile and build it with the ida-cmake agent.

---

# eject_idb - Emergency IDB Save Plugin for IDA Pro

A last-ditch emergency plugin to save your IDB when IDA hangs or encounters critical errors. Operates via inter-process communication using named semaphores.

## Project Overview

**Purpose**: Rescue unsaved IDA Pro work when the UI hangs or encounters fatal errors (infinite loops, access violations, API misuse, etc.).

**Mechanism**: Spawns a background thread that waits on a per-database named semaphore. When an external executable signals the semaphore, the plugin saves the current IDB as `original-name.ejected.ext` and optionally terminates IDA.

**Components**:
1. **eject_plugin** (IDA plugin DLL) - Background thread monitoring for eject signal
2. **eject_idb** (standalone executable) - CLI tool to trigger the eject

**Status**: Production-ready but with known limitations (not foolproof, may fail in severe crashes).

## Architecture

### Two-Component System

The system consists of two separate executables that communicate via named semaphores:

1. **eject_idb.exe** - CLI tool that signals the semaphore when invoked
2. **eject_plugin.dll** - IDA plugin with background thread monitoring the semaphore
3. When signaled, the plugin saves the IDB as `*.ejected.*` and optionally exits IDA

### Semaphore Naming

Both components use a shared naming scheme based on djb2 hash of the IDB path:

```cpp
// utils.hpp
inline void make_semaphore_name(const char* string, char* out, size_t out_size) {
    snprintf(out, out_size, "ejectidb_%08lx", djb2(string));
}
```

**Format**: `ejectidb_XXXXXXXX` where `XXXXXXXX` is the 8-character hex hash of the full IDB path.

**Important**: IDB path is case-sensitive on all platforms when generating the hash.

### Plugin Thread Model

The plugin (`eject_plugin.cpp`) uses a dedicated background thread:

```cpp
class semaphore_helper_t {
    std::thread thread_;        // Background worker
    qsemaphore_t sem_;         // IDA SDK semaphore
    std::function<void()> callback_;  // Eject handler
    bool should_exit_;         // Clean shutdown flag
};
```

**Thread lifecycle**:
1. Created in `plugin_ctx_t` constructor
2. Waits indefinitely on `qsem_wait(sem_, -1)`
3. Invokes `do_eject()` when signaled
4. Terminated gracefully in destructor

### Eject Process Flow

1. **External trigger**: User runs `eject_idb.exe "C:\path\to\database.idb"`
2. **Semaphore signal**: Executable posts to the named semaphore
3. **Plugin wakes**: Background thread receives signal
4. **UI blocking**: Sets `disable_ui = true` to suppress IDA's save dialogs
5. **Save IDB**: Calls `save_database(new_name.c_str(), DBFL_BAK)`
6. **Optional exit**: Windows MessageBox asks user if they want to forcefully exit IDA
7. **Cleanup**: Restores `disable_ui = false`

### UI Message Suppression

The plugin hooks `HT_UI` notifications to block UI messages during save:

```cpp
static ssize_t idaapi ui_callback(void* ud, int notification_code, va_list va) {
    plugin_ctx_t* ctx = (plugin_ctx_t*)ud;
    return ctx->disable_ui ? 1 : 0;  // Return 1 to suppress UI
}
```

**Why necessary**: `save_database()` calls into the main thread to display success/failure dialogs, which could deadlock if the UI is already hung.

## File Structure

- **eject_plugin.cpp** - IDA plugin (background thread + eject logic)
- **eject_idb.cpp** - CLI executable (semaphore signaler)
- **utils.hpp** - Shared utilities (semaphore naming, djb2 hash)
- **CMakeLists.txt** - Build configuration (modern ida-cmake)
- **README.md** - User documentation
- **prep-cmake.bat** - Windows build helper
- **build/** - Build output directory

## Build System

### Modern ida-cmake

The project uses modern ida-cmake (9.2+) with the `ida_add_plugin()` convenience function:

```cmake
include($ENV{IDASDK}/ida-cmake/bootstrap.cmake)
find_package(idasdk REQUIRED)

ida_add_plugin(eject_plugin
    SOURCES
        eject_plugin.cpp
        utils.hpp
    DEBUG_ARGS
        "-t"
        "-z10000"
)
```

**Environment variable required**: `IDASDK` must point to the IDA SDK directory.

### Building

```bash
# Configure and build
cmake -B build
cmake --build build --config RelWithDebInfo

# Deploy both plugin and executable to IDA directories
cmake --install build --config RelWithDebInfo
```

**Deployment**:
- **Plugin**: Automatically deployed to `$IDABIN/plugins/eject_plugin.dll` by ida-cmake
- **Executable**: Deployed to `$IDABIN/eject_idb.exe` via CMake install target

**Notes**:
- Uses C++17 standard (compatible with idacpp which requires C++20)
- Modern ida-cmake automatically handles architecture detection and deployment
- Both components are built and deployed with a single install command

## Platform Support

### Windows (`_WIN32`)
- Semaphores: Win32 named semaphores (`CreateSemaphore`, `OpenSemaphore`)
- UI: MessageBox for exit prompt
- Process exit: `ExitProcess(0)`

### macOS/Linux (`__APPLE__` || `__linux__`)
- Semaphores: POSIX named semaphores (`sem_open`, `sem_post`)
- UI: No exit prompt (automatically returns after save)
- Process exit: Not implemented (returns to IDA)

**Platform detection**:
- Plugin: `#ifdef __NT__` (IDA SDK macro for Windows)
- Executable: `#ifdef _WIN32` (standard compiler macro)

## Key Implementation Details

### Semaphore Lifecycle (Plugin Side)

```cpp
// Created with initial count 0
sem_ = qsem_create(sem_name_.c_str(), 0);

// Wait indefinitely (-1 = infinite timeout)
while (qsem_wait(sem_, -1)) {
    if (should_exit_) break;
    callback_();  // do_eject()
}

// Cleanup
qsem_free(sem_);
```

### Semaphore Signaling (CLI Side)

```cpp
// Windows
HANDLE hSemaphore = OpenSemaphoreA(SEMAPHORE_MODIFY_STATE, FALSE, sem_name);
ReleaseSemaphore(hSemaphore, 1, NULL);
CloseHandle(hSemaphore);

// POSIX
sem_t* sem = sem_open(sem_name, 0);
sem_post(sem);
sem_close(sem);
```

**Important**: CLI doesn't create the semaphore, only opens existing one. Plugin must be loaded first.

### Output Filename Generation

```cpp
qstring p = get_idb_path();  // "C:\path\to\database.idb"
auto idx = p.rfind('.');     // Find last '.'
qstring new_name = p.substr(0, idx) + ".ejected" + p.substr(idx);
// Result: "C:\path\to\database.ejected.idb"
```

**Extension preserved**: `.idb`, `.i64`, `.til`, etc. all become `.ejected.idb`, `.ejected.i64`, `.ejected.til`.

## Testing Features

The plugin includes test modes accessible via `Edit > Plugins > eject_idb`:

### Mode 0: Non-responsive UI Hang
```cpp
while (true) {
    qsleep(1000);  // No user_cancelled() polling
}
```
Simulates a completely frozen UI (most realistic test).

### Mode 1: Responsive UI Hang
```cpp
while (true) {
    user_cancelled();  // Poll for cancellation
    qsleep(1000);
}
```
Simulates a responsive but infinite loop (less realistic).

**Testing with `__TESTING__` define**: `eject_idb.cpp` can be compiled with `__TESTING__` to add a manual pause before signaling:

```cpp
#ifdef __TESTING__
    printf("eject_idb waiting...press ENTER to continue\n");
    fgets(input, sizeof(input), stdin);
#endif
```

## Known Limitations

1. **Not foolproof**: May fail in severe crashes (memory corruption, stack overflow, etc.)
2. **Windows-only exit**: Only Windows gets the MessageBox exit prompt
3. **Main thread dependency**: `save_database()` still calls into main thread - if main thread is completely deadlocked, save may fail
4. **Single-instance**: No support for multiple IDA instances with same IDB path (semaphore name collision)
5. **No auto-recovery**: Doesn't automatically detect hangs; user must manually trigger

## Usage Pattern

```bash
# 1. IDA loads the plugin (automatic)
# 2. Plugin prints:
#    "eject_idb installed. call the 'eject_idb "C:\path\to\database.idb"' command line tool to eject this database!"

# 3. When IDA hangs, run from another terminal:
eject_idb.exe "C:\full\path\to\database.idb"

# 4. Plugin wakes, saves as "database.ejected.idb"
# 5. (Windows only) MessageBox asks if you want to forcefully exit IDA
```

**Important**: Path must be exact, including case and extension.

## TODOs (from source comments)

```cpp
// eject_plugin.cpp:19-20
// TODO:
// - use on_event/modern mechanism
// - delay event handler installation with a timer. give time to other plugins to install their handlers; come in last
```

**Explanation**:
- Currently uses old `PLUGIN` struct pattern
- Should migrate to modern `plugmod_t` pattern (already partially there)
- UI hook timing: Installing the UI hook last would ensure eject_idb's handler runs before others (handler chain is LIFO)

## Integration with Other Projects

### idacpp
Not currently used. Could modernize with:
- `idacpp::core::objcontainer_t` for semaphore management
- Modern C++20 patterns
- Requires bumping to C++20 standard

### QScripts
Can be used together. If QScripts causes a hang, eject_idb can still save.

### ida-cmake
Now uses modern ida-cmake (9.2+) with `ida_add_plugin()` convenience function. Automatically handles:
- Platform/architecture detection
- Compiler flags and warnings suppression
- Deployment to `$IDABIN/plugins/`
- Debug configurations for IDEs

## Common Issues

### "Failed to eject"
**Causes**:
1. IDB path doesn't match exactly (case-sensitive)
2. Plugin not loaded (semaphore doesn't exist)
3. Wrong architecture (32-bit plugin vs 64-bit IDA or vice versa)

**Solution**: Check plugin loaded, verify path case, ensure architecture matches.

### Save Succeeds but IDA Doesn't Exit
**Expected behavior on Linux/macOS**: No exit prompt implemented.

**On Windows**: User chose "No" in the MessageBox.

### Plugin Doesn't Load
**Causes**:
1. Architecture mismatch (32-bit DLL in 64-bit IDA)
2. Missing dependencies (unlikely - uses only standard SDK)
3. IDA version incompatibility

**Solution**: Check build architecture, rebuild with correct IDA SDK version.

## Development Guidelines

### When Modifying

1. **Thread safety**: All shared state must be synchronized
2. **Semaphore naming**: Changing hash function breaks CLI compatibility
3. **Platform testing**: Test on Windows, macOS, and Linux
4. **UI suppression**: Any `save_database()` call must suppress UI to avoid deadlocks
5. **Clean shutdown**: Thread must be joinable in destructor

### When Building

```bash
# Configure and build
cmake -B build
cmake --build build --config RelWithDebInfo

# Deploy to IDA installation
cmake --install build --config RelWithDebInfo
```

**What gets deployed**:
- `eject_plugin.dll` → `$IDABIN/plugins/` (automatic via ida-cmake)
- `eject_idb.exe` → `$IDABIN/` (via CMake install target)

### When Testing

1. Load IDA with test database
2. Trigger test mode (Edit > Plugins > eject_idb > Mode 0)
3. From external terminal: `eject_idb.exe "C:\full\path\to\test.idb"`
4. Verify `test.ejected.idb` exists
5. Verify contents are valid (can be opened in IDA)

## References

- **djb2 hash**: Classic hash function by Dan Bernstein (comp.lang.c)
- **IDA semaphores**: `<pro.h>` - `qsem_create`, `qsem_wait`, `qsem_post`, `qsem_free`
- **Win32 semaphores**: `<windows.h>` - `CreateSemaphore`, `OpenSemaphore`, `ReleaseSemaphore`
- **POSIX semaphores**: `<semaphore.h>` - `sem_open`, `sem_post`, `sem_close`
- **IDA database API**: `<loader.h>` - `save_database`, `get_path(PATH_TYPE_IDB)`

## Quick Reference

### Key Functions

**Plugin**:
- `plugin_ctx_t::do_eject()` - Main eject logic (eject_plugin.cpp:88)
- `semaphore_helper_t::start()` - Spawn background thread (eject_plugin.cpp:40)
- `ui_callback()` - Suppress UI messages (eject_plugin.cpp:82)

**CLI**:
- `main()` - Signal semaphore (eject_idb.cpp:16)

**Shared**:
- `make_semaphore_name()` - Generate semaphore name from IDB path (utils.hpp:26)
- `djb2()` - Hash function (utils.hpp:18)

### Important Constants

- Semaphore prefix: `"ejectidb_"`
- Initial semaphore count: `0`
- Infinite wait timeout: `-1`
- Save flags: `DBFL_BAK`
- Output suffix: `".ejected"`
