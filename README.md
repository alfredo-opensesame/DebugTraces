# DebugTracing Library

A portable, thread-safe C++20 logging library with compile-time optimization and runtime configuration.

## Project Structure

```
src/
├── DebugTracesLib.h    # Minimal API header with 7 macros and per-file controls
└── Logger.cpp          # Thread-safe implementation with fixed buffers

tests/
├── dbgtracing_gtest.cpp      # Core functionality tests
├── test_disabled.cpp         # Disabled logging tests
├── test_enabled.cpp          # Enabled logging tests
├── test_thread_format.cpp    # Thread formatting tests
├── test_thread_id_format.cpp # Thread ID format tests
└── test_trace_macros.cpp     # Comprehensive macro tests

cmake/
└── DebugTracingConfig.cmake.in # CMake package configuration
```

## Features

- **Minimal API**: Only 7 macros (`LOGF`, `LOGE`, `LOGW`, `LOGI`, `TRACE`, `LOGTIMER`, `LOG_FILE`)
- **Per-File Override**: `TX_TRACE_THIS_FILE` controls logging per compilation unit
- **Compile-Time Stripping**: Disabled logging compiles to no-ops (zero overhead)
- **Thread-Safe**: Mutex-protected logging with no heap allocations in hot paths
- **Dual Output**: Console (stderr) and optional file output with configurable paths
- **RAII Timing**: `LOGTIMER(name)` automatically logs scope duration
- **Cross-Platform**: Works on macOS, iOS, Linux, Windows
- **C/C++ Compatible**: Works in both C and C++ projects

## Main Macros Reference

### **Available Macros (Minimal API)**
| Macro | Level | Description |
|-------|-------|-------------|
| `LOGF(fmt, ...)` | FATAL | Critical errors that cause termination |
| `LOGE(fmt, ...)` | ERROR | Runtime errors that affect functionality |
| `LOGW(fmt, ...)` | WARN | Potential issues or degraded performance |
| `LOGI(fmt, ...)` | INFO | General informational messages |
| `TRACE(fmt, ...)` | DEBUG | Basic debug tracing (controlled by TX_TRACE_THIS_FILE) |

### **Special Macros**
| Macro | Description |
|-------|-------------|
| `LOGTIMER(name)` | RAII scope timer that logs duration on destruction |
| `LOG_TO_FILE(...)` | Enable file logging (defaults to "debugtraces.log" if empty) |

### **Compile-Time Controls**
| Macro | Purpose |
|-------|---------|
| `TX_TRACE_ENABLED` | Global enable/disable (default: 1 in Debug, 0 in Release) |
| `TX_TRACE_THIS_FILE` | Per-file override (0/1, overrides global setting) |
| `TX_LOG_TO_FILE` | Default file logging state (default: 0) |
| `TX_LOG_FILE_PATH` | Default log file path (default: "debugtraces.log") |

### **API Functions**
| Function | Description |
|----------|-------------|
| `tx_log_set_file_path(path)` | Set log file path (nullptr/"" uses "debugtraces.log") |
| `tx_log_enable_file(enabled)` | Enable/disable file logging |
| `tx_log_is_enabled()` | Check if logging is enabled |
| `tx_log_flush()` | Flush file output buffer |

## Basic Usage

### Simple Logging
```cpp
#include "DebugTracesLib.h"

int main() {
    // Enable file logging
    LOG_TO_FILE();  // Uses "debugtraces.log"
    // Or specify custom file:
    // LOG_TO_FILE("myapp.log");
    
    // Basic logging macros
    LOGI("Application started");
    LOGW("Warning: value=%d", 42);
    LOGE("Error occurred: %s", "file not found");
    LOGF("Fatal error - terminating");
    TRACE("Debug trace message");
    
    return 0;
}
```

### Per-File Control
```cpp
// Disable logging for this file
#define TX_TRACE_THIS_FILE 0
#include "DebugTracesLib.h"
// All macros are now no-ops in this file

// OR enable logging (inherits global setting by default)
#define TX_TRACE_THIS_FILE 1  
#include "DebugTracesLib.h"
// All macros are active in this file
```

### RAII Timing
```cpp
#include "DebugTracesLib.h"

void processAudio() {
    LOGTIMER("AudioProcessing");  // Logs duration when scope exits
    
    // Your processing code here...
    
    // Timer automatically logs: "AudioProcessing took 42.123 ms"
}

void nestedTimers() {
    LOGTIMER("OuterOperation");
    {
        LOGTIMER("InnerOperation");
        // Inner work...
    }  // Inner timer logs first
    // More outer work...
}  // Outer timer logs second
```

### File Output Control
```cpp
#include "DebugTracesLib.h"

void configureLogging() {
    // Method 1: Use LOG_TO_FILE macro (recommended)
    LOG_TO_FILE("debug.log");          // Custom filename
    LOG_TO_FILE();                     // Default "debugtraces.log"
    
    // Method 2: Direct API calls
    tx_log_set_file_path("custom.log");
    tx_log_enable_file(true);
    
    // Disable file output
    tx_log_enable_file(false);
    
    // Force flush
    tx_log_flush();
}
```

## Advanced Features

### Per-File Override System

The key feature is **per-file override** of the global logging state:

```cpp
// File A: Disable logging entirely (all macros become no-ops)
#define TX_TRACE_THIS_FILE 0  
#include "DebugTracesLib.h"

void criticalPath() {
    LOGI("This won't log");        // Compiled to no-op
    TRACE("This won't log either"); // Compiled to no-op
    LOGTIMER("timing");           // Compiled to no-op
}
```

```cpp
// File B: Enable logging (overrides global setting)
#define TX_TRACE_THIS_FILE 1
#include "DebugTracesLib.h"

void debugCode() {
    LOGI("This will log");         // Active
    TRACE("Debug information");    // Active
    LOGTIMER("operation");        // Active - will log timing
}
```

### Global Compile-Time Control

Control the default behavior across your project:

```cpp
// In CMake or compiler flags:
// -DTX_TRACE_ENABLED=0    // Disable globally (Release builds)
// -DTX_TRACE_ENABLED=1    // Enable globally (Debug builds)

// Default behavior (automatic):
// Debug builds:   TX_TRACE_ENABLED=1 (logging active)
// Release builds: TX_TRACE_ENABLED=0 (logging compiled out)
```

### Build System Integration

```cmake
# CMakeLists.txt - Global control
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    add_compile_definitions(TX_TRACE_ENABLED=1)
    add_compile_definitions(TX_LOG_TO_FILE=1)
else()
    add_compile_definitions(TX_TRACE_ENABLED=0)
endif()

# Optional: Set global file path
add_compile_definitions(TX_LOG_FILE_PATH="app_debug.log")
```

### Output Format

Log lines follow this format:
```
EPOCH_MS | TID | LEVEL | file:line func() | message
```

Example output:
```
1698765432123 | a1b2c3d4 | INFO | main.cpp:15 main() | Application started
1698765432124 | a1b2c3d4 | WARN | audio.cpp:42 processBuffer() | Buffer underrun detected
1698765432156 | a1b2c3d4 | INFO | audio.cpp:55 processBuffer() | AudioProcessing took 32.456 ms
```

## CMake Integration

### Option A: As a Subdirectory (Recommended)
```cmake
# Add to your CMakeLists.txt
add_subdirectory(path/to/DebugTraces)
target_link_libraries(YourTarget PRIVATE DBGTX::dbgtracing)
```

### Option B: Find Installed Package
```cmake
# First install the library
find_package(DebugTracing REQUIRED)
target_link_libraries(YourTarget PRIVATE DBGTX::dbgtracing)
```

### Build Options
```cmake
# Enable tests (requires Google Test)
# Creates a single dbgtracing_tests executable with all test files
set(DBGTR_BUILD_TESTS ON)

# Disable installation
set(DBGTR_INSTALL OFF)

# Enable extra warnings
set(DBGTR_WARNINGS ON)

# Treat warnings as errors  
set(DBGTR_WERROR ON)
```

## Building

### Quick Start
```bash
# Configure
cmake -B build -S .

# Build
cmake --build build

# Run tests (if enabled)
cmake --build build --target test
```

### With Tests
```bash
# Configure with tests enabled
mkdir -p build && cd build
cmake .. -DDBGTR_BUILD_TESTS=ON

# Build and run tests
cmake --build .
ctest --output-on-failure

# Or run the test executable directly
./dbgtracing_tests
```

## Usage Examples

### Real-time Audio Thread
```cpp
// High-performance path - disable logging
#define TX_TRACE_THIS_FILE 0
#include "DebugTracesLib.h"

void audioCallback() {
    TRACE("This compiles to nothing");  // Zero overhead
    LOGE("Even errors compile to nothing"); // Zero overhead
}
```

### Development/Debug Code
```cpp
// Development path - enable logging  
#define TX_TRACE_THIS_FILE 1
#include "DebugTracesLib.h"

void initAudio() {
    LOG_TO_FILE("audio_debug.log");
    LOGI("Audio system initializing");
    
    {
        LOGTIMER("DeviceSetup");
        setupAudioDevice();  // Timer logs duration automatically
    }
    
    LOGI("Audio system ready");
}
```

### Error Conditions Only
```cpp
// Production path - errors only
#define TX_TRACE_THIS_FILE 1  // Enable logging
#include "DebugTracesLib.h"

void processData() {
    // LOGI and TRACE calls for normal operation removed in production
    
    if (errorCondition) {
        LOGE("Data processing failed: %s", errorMsg);  // Keep for debugging
        LOGF("Critical failure - cannot continue");   // Keep fatal errors
    }
}
```

## Requirements

- **C++11** or higher (for TxScopeTimer RAII, C interface works with C99)
- **CMake 3.20** or higher  
- **Google Test** (optional, for tests only)
- **Ninja** or **Make** build system
