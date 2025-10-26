# DebugTracing Logging Library (Portable, Thread-Safe)

This library provides:
- `DebugTracesLib.h`: public logging macros (`TX_LOG_INFO`, `TX_LOG_WARN`, `TX_LOG_ERROR`, `TX_LOG_DEBUG`) and portable helpers.
- `Logger.[h/cpp]`: a thread-safe singleton (lazy; no manual start/stop required) logger that emits to the platform debugger and optionally to a file.
- `Writer.[h/cpp]`: a background file writer that trims the log file by line count.

## Highlights
- **Portable sinks**: OutputDebugStringA on Windows; syslog + stderr on macOS/Linux.
- **Thread-safe**: internal mutexes and atomics; background writer with condition_variable.
- **No dangling refs**: thread-safe singleton (lazy; no manual start/stop required) avoids manual ref-counting.
- **Line-capped file**: when `maxLines > 0`, the writer keeps the last 70% of lines when trimming.

## Basic global usage

```cpp
#include "TX_DebugTracesLib.h"
#include "TX_Logger.h"

int main() {
  TX_Logger::getPtr()->setLogLevel(LogLevel::LL_DEBUG);
  TX_Logger::getPtr()->logIntoFile("app.log", /*maxLines=*/10000);

  TX_LOG_INFO("Hello %s", "world");
  TX_LOG_WARN("Value=%d", 42);
  TX_LOG_ERROR("Something failed: code=%d", -1);
}
```

## Per-file controls

At the very top of a `.cpp` file you can enable tracing banners and override log level:

```cpp
// Optional: banner at compile time
#define TX_TRACE_THIS_FILE 1

// Optional: raise/lower verbosity only for this file
#define TX_LOG_LEVEL_FILE TX_LogLevel::Debug

#include "TX_DebugTracesLib.h"
#include "TX_Logger.h"

void foo() {
  TX_LOG_DEBUG("This will appear only if this file's log level includes Debug");
}
```

On MSVC this will emit a `#pragma message` banner; on GCC/Clang it will emit a `#warning` if `TX_TRACE_THIS_FILE` is set.

## CMake Integration

### Option A: As a subdirectory (simplest for local sources)
```cmake
add_subdirectory(path/to/DebugTraces DebugTraces-build)
target_link_libraries(YourTarget PRIVATE DBGTX::dbgtracing)
```

### Option B: Installed + find_package()
```cmake
find_package(DebugTracing REQUIRED)
target_link_libraries(YourTarget PRIVATE DBGTX::dbgtracing)
```
