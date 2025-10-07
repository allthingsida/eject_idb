# Eject IDB Plugin for IDA

The `eject_idb` plugin was conceived after discussions with friends about the frustrations of IDA's UI hanging/IDA internal errors without warning or due to programmer errors (like infinite loops, access violations, or API misuse in their plugins or scripts), leaving no opportunity to save ongoing work. Worry no more! `eject_idb` might *potentially* save the day.

It operates by spawning a separate thread that waits for a per-database semaphore to be signaled. When signaled, the plugin saves your current IDB as `original-name.ejected.ext`, then offers to terminate IDA on your behalf.

In short, `eject_idb` can be used as a last-ditch effort to save your work before IDA crashes.

**Note: This ejection mechanism is not foolproof and might not work in all scenarios**

## Building

Prerequisites:
- CMake 3.27+
- IDA SDK 9.2+ (set `IDASDK` environment variable)
- ida-cmake (installed correctly)
- Visual Studio 2022 (Windows) / GCC/Clang (Linux/macOS)

```bash
# Configure and build
cmake -B build
cmake --build build --config RelWithDebInfo

# Deploy to IDA installation
cmake --install build --config RelWithDebInfo
```

This will deploy:
- `eject_plugin.dll` -> `$IDABIN/plugins/`
- `eject_idb.exe` -> `$IDABIN/`

Alternatively, download pre-built binaries from the [releases page](https://github.com/0xeb/allthingsida/releases).

## Usage

1. The plugin activates automatically when you open an IDB
2. The plugin prints: `eject_idb installed. call the 'eject_idb "<path>"' command line tool to eject this database!`
3. When IDA hangs, run from an external terminal:
   ```bash
   eject_idb "C:\full\path\to\database.idb"
   ```
4. The plugin wakes up, saves as `database.ejected.idb`
5. (Windows only) A dialog prompts whether to forcefully exit IDA

**Important**: The IDB path must match exactly (case-sensitive, full path).

To simulate and test a crash, run the plugin with arg=1, like this:

```python
idaapi.load_and_run_plugin("eject_plugin", 1)
```
