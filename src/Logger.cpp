// DebugTraces.cpp - Minimal API implementation
#include "DebugTracesLib.h"
#include <cstdarg>
#include <cstdio>
#include <cstring>
#include <mutex>
#include <chrono>
#include <thread>
#include <fstream>
#include <string>
#include <atomic>
#include <iomanip>
#include <sstream>

#if TX_TRACE_ENABLED

// Global state
static std::mutex g_log_mu;
static std::string g_file_path = TX_LOG_FILE_PATH;
static std::atomic<bool> g_file_enabled{false};  // Only log to file if LOG_TO_FILE() is called
static std::ofstream g_file_stream;

// RAII cleanup class to close file on process exit
class LogFileCleanup {
public:
    ~LogFileCleanup() {
        std::lock_guard<std::mutex> lock(g_log_mu);
        if (g_file_stream.is_open()) {
            g_file_stream.close();
        }
    }
};
static LogFileCleanup g_cleanup;  // Automatically closes file on exit

// Helper to get formatted datetime string
static std::string get_datetime() {
    auto now = std::chrono::system_clock::now();
    auto time_t = std::chrono::system_clock::to_time_t(now);
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()) % 1000;
    
    std::stringstream ss;
    ss << std::put_time(std::localtime(&time_t), "%Y-%m-%d %H:%M:%S");
    ss << '.' << std::setfill('0') << std::setw(3) << ms.count();
    return ss.str();
}

// Helper to get thread ID when needed
static uint64_t get_thread_id() {
    std::hash<std::thread::id> hasher;
    return hasher(std::this_thread::get_id());
}

// ANSI color codes for different log levels
static const char* get_level_color(TxLogLevel lvl) {
    switch (lvl) {
        case TxLogLevel::FATAL: return "\033[1;31m";  // Bright Red
        case TxLogLevel::ERROR: return "\033[0;31m";  // Red
        case TxLogLevel::WARN:  return "\033[0;33m";  // Yellow
        case TxLogLevel::INFO:  return "\033[0;32m";  // Green
        case TxLogLevel::DBG: return "";              // No color for TRACE/DEBUG
        default: return "\033[0m";                    // Reset
    }
}

// Level strings (excluding DEBUG per requirements)
static const char* get_level_string(TxLogLevel lvl) {
    switch (lvl) {
        case TxLogLevel::FATAL: return "FATAL";
        case TxLogLevel::ERROR: return "ERROR";
        case TxLogLevel::WARN:  return "WARN";
        case TxLogLevel::INFO:  return "INFO";
        case TxLogLevel::DBG: return "";    // Empty string for DBG level
        default: return "UNKNOWN";
    }
}

extern "C" {

void tx_log_emit(TxLogLevel lvl, const char* file, int line,
                 const char* /* func */, const char* fmt, ...) {
    std::lock_guard<std::mutex> lock(g_log_mu);
    
    // Format message
    char message[2048];
    va_list args;
    va_start(args, fmt);
    vsnprintf(message, sizeof(message), fmt, args);
    va_end(args);
    
    // Extract basename from file path
    const char* basename = strrchr(file, '/');
    if (!basename) basename = strrchr(file, '\\');
    if (!basename) basename = file; else ++basename;
    
    // Get datetime
    std::string datetime = get_datetime();
    
    // Get level string and color
    const char* level_str = get_level_string(lvl);
    const char* color = get_level_color(lvl);
    const char* reset_color = "\033[0m";
    
    // Format log line based on TRACE_THREAD_ID and level requirements
    char logline_console[4096];
    char logline_file[4096];  // File output without ANSI colors
    
#if TRACE_THREAD_ID
        // Format: <DateTime> [<Level>][<ThreadID>] <filename>:<line>: <Message>
        uint64_t thread_id = get_thread_id();
        if (strlen(level_str) > 0) {
            // Non-DEBUG levels with color
            snprintf(logline_console, sizeof(logline_console), 
                     "%s %s[%s]%s[%llx] %s:%d: %s\n",
                     datetime.c_str(), color, level_str, reset_color, 
                     thread_id, basename, line, message);
            snprintf(logline_file, sizeof(logline_file), 
                     "%s [%s][%llx] %s:%d: %s\n",
                     datetime.c_str(), level_str, thread_id, basename, line, message);
        } else {
            // DEBUG level (no level tag)
            snprintf(logline_console, sizeof(logline_console), 
                     "%s %s[%llx]%s %s:%d: %s\n",
                     datetime.c_str(), color, thread_id, reset_color, 
                     basename, line, message);
            snprintf(logline_file, sizeof(logline_file), 
                     "%s [%llx] %s:%d: %s\n",
                     datetime.c_str(), thread_id, basename, line, message);
        }
#else
        // Format: <DateTime> [<Level>] <filename>:<line>: <Message>
        if (strlen(level_str) > 0) {
            // Non-DEBUG levels with color
            snprintf(logline_console, sizeof(logline_console), 
                     "%s %s[%s]%s %s:%d: %s\n",
                     datetime.c_str(), color, level_str, reset_color, 
                     basename, line, message);
            snprintf(logline_file, sizeof(logline_file), 
                     "%s [%s] %s:%d: %s\n",
                     datetime.c_str(), level_str, basename, line, message);
        } else {
            // DEBUG level (no level tag)
            snprintf(logline_console, sizeof(logline_console), 
                     "%s %s%s:%d: %s%s\n",
                     datetime.c_str(), color, basename, line, message, reset_color);
            snprintf(logline_file, sizeof(logline_file), 
                     "%s %s:%d: %s\n",
                     datetime.c_str(), basename, line, message);
        }
#endif
    
    // Output to console with colors
    fprintf(stderr, "%s", logline_console);
    
    // Output to file without colors if enabled
    if (g_file_enabled.load()) {
        if (!g_file_stream.is_open()) {
            g_file_stream.open(g_file_path, std::ios::app);
        }
        if (g_file_stream.is_open()) {
            g_file_stream << logline_file;
            g_file_stream.flush();
        }
    }
}

void tx_log_set_file_path(const char* path) {
    std::lock_guard<std::mutex> lock(g_log_mu);
    g_file_path = (path && path[0]) ? path : "debugtraces.log";
    if (g_file_stream.is_open()) {
        g_file_stream.close();
    }
}

void tx_log_enable_file(bool enabled) {
    g_file_enabled.store(enabled);
    if (!enabled) {
        std::lock_guard<std::mutex> lock(g_log_mu);
        if (g_file_stream.is_open()) {
            g_file_stream.close();
        }
    }
}

bool tx_log_is_enabled() {
    return true;  // Always enabled when TX_TRACE_THIS_FILE is 1
}

void tx_log_flush() {
    std::lock_guard<std::mutex> lock(g_log_mu);
    if (g_file_stream.is_open()) {
        g_file_stream.flush();
    }
}

} // extern "C"

// Helper to get nanosecond timestamp for timing
static unsigned long long get_timestamp_ns() {
    auto now = std::chrono::steady_clock::now();
    auto ns = std::chrono::duration_cast<std::chrono::nanoseconds>(now.time_since_epoch());
    return ns.count();
}

// TxScopeTimer implementation
TxScopeTimer::TxScopeTimer(const char* n, const char* f, int l, const char* fn) noexcept
    : name(n), file(f), func(fn), line(l), t0_ns(get_timestamp_ns()) {
}

TxScopeTimer::~TxScopeTimer() noexcept {
    unsigned long long t1_ns = get_timestamp_ns();
    double duration_ms = (t1_ns - t0_ns) / 1000000.0;
    tx_log_emit(TxLogLevel::INFO, file, line, func, "%s took %.3f ms", name, duration_ms);
}

#endif // TX_TRACE_ENABLED
