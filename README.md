# DebugTracing Library

A portable, thread-safe C++20 logging library with compile-time optimization and runtime configuration.

## Project Structure

```
src/
├── DebugTracesLib.h    # Main header with logging macros and portable helpers
├── Logger.h/cpp        # Thread-safe singleton logger with multiple output sinks
└── Writer.h/cpp        # Background file writer with automatic log rotation

tests/
└── dbgtracing_gtest.cpp # Google Test suite

cmake/
└── DebugTracingConfig.cmake.in # CMake package configuration
```

## Features

- **Multiple Log Levels**: FATAL, ERROR, WARN, INFO, DEBUG, TRACE
- **Compile-time Optimization**: Log statements can be compiled out based on level
- **Multiple Output Sinks**: 
  - Platform debugger (OutputDebugString on Windows, syslog on Unix)
  - Console/stderr output
  - File output with automatic rotation
- **Thread-Safe**: Uses mutexes and condition variables for safe multi-threaded logging  
- **Per-File Controls**: Enable tracing and time measurement on a per-compilation-unit basis
- **Automatic Log Rotation**: Keeps last 70% of lines when file size limit is reached
- **Portable**: Works on Windows (MSVC), macOS, and Linux (GCC/Clang)

## Basic Usage

### Simple Logging
```cpp
#include "DebugTracesLib.h"
#include "Logger.h"

int main() {
    // Get logger instance and configure
    auto* logger = TX_Logger::getPtr();
    logger->setLogLevel(LogLevel::LL_DEBUG);
    logger->logIntoFile("app.log", /*maxLines=*/10000);
    
    // Use convenient macros
    TX_LOG_INFO("Application started");
    TX_LOG_WARN("Warning: value=%d", 42);
    TX_LOG_ERROR("Error occurred: %s", "file not found");
    TX_LOG_DEBUG("Debug info: %p", &logger);
    
    return 0;
}
```

### Alternative Macro Style
```cpp
#include "Logger.h"

void someFunction() {
    LogInfo("Information message");
    LogWarn("Warning message"); 
    LogError("Error message");
    LogDebug("Debug message");  // Only in debug builds
}
```

### Stream-Style Logging
```cpp
#include "Logger.h"

void streamExample() {
    TraceInfo() << "Stream info: " << 123;
    TraceWarning() << "Stream warning: " << "test";  
    TraceError() << "Stream error: " << 456;
}
```

## Advanced Features

### Per-File Controls

Define these macros **before** including the header to control per-compilation-unit behavior:

```cpp
// Enable tracing for this file (shows compile-time banner)
#define TX_TRACE_THIS_FILE 1

// Enable time measurement helpers (debug builds only)  
#define TX_MEASURE_TIME 1

// Override log level for this specific file
#define TX_LOG_LEVEL_FILE TX_LVL_DEBUG

#include "DebugTracesLib.h"

void myFunction() {
    TRACE("This will log when TX_TRACE_THIS_FILE is enabled");
    TX_LOG_DEBUG("File-specific debug message");
}
```

### Time Measurement

```cpp
#define TX_MEASURE_TIME 1
#include "DebugTracesLib.h"

void timedOperation() {
    TX_START_TIME_MEASUREMENT();
    
    // Your code here
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    
    TX_PRINT_TIME_MEASUREMENT(); // Logs elapsed time in microseconds
}

void namedTimingContext() {
    TX_START_TIME_MEAS_CTX(database_query);
    
    // Database operation
    
    TX_PRINT_TIME_MEAS_CTX(database_query); // Logs "Time Elapsed (database_query): X us"
}
```

### Compile-Time Log Level Control

```cpp
// Global log level (affects all files)
#define TX_LOG_LEVEL_GLOBAL TX_LVL_WARN  // Only WARN and ERROR will be compiled

// File-specific override
#define TX_LOG_LEVEL_FILE TX_LVL_DEBUG   // This file gets DEBUG and above

#include "DebugTracesLib.h"
```

Available levels: `TX_LVL_DEBUG`, `TX_LVL_INFO`, `TX_LVL_WARN`, `TX_LVL_ERROR`

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
cmake -B build -S . -DDBGTR_BUILD_TESTS=ON

# Build and run tests
cmake --build build
cd build && ctest
```

## Requirements

- **C++20** compatible compiler
- **CMake 3.20** or higher  
- **Google Test** (optional, for tests only)
- **Ninja** or **Make** build system
