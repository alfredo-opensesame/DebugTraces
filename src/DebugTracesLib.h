#pragma once
#include <stdint.h>
#include "TraceColors.h"

// ================= Global compile-time controls =================
#ifndef TX_TRACE_ENABLED
  #if defined(NDEBUG)
    #define TX_TRACE_ENABLED 0
  #else
    #define TX_TRACE_ENABLED 1
  #endif
#endif

#ifndef TX_LOG_TO_FILE
  #define TX_LOG_TO_FILE 0
#endif

#ifndef TX_LOG_FILE_PATH
  #define TX_LOG_FILE_PATH "debugtraces.log"
#endif

#ifndef TRACE_THREAD_ID
  #define TRACE_THREAD_ID 0
#endif

// ================= Per-file override =================
// If the translation unit defines TX_TRACE_THIS_FILE (0/1) before including,
// it overrides the global TX_TRACE_ENABLED.
#ifndef TX_TRACE_THIS_FILE
  #define TX_TRACE_THIS_FILE TX_TRACE_ENABLED
#endif

// ================= Public API =================
#if TX_TRACE_THIS_FILE

enum class TxLogLevel : uint8_t { FATAL=0, ERROR=1, WARN=2, INFO=3, DBG=4 };

#ifdef __cplusplus
extern "C" {
#endif

// printf-style sink (implemented in DebugTraces.cpp), thread-safe, no heap
void tx_log_emit(TxLogLevel lvl, const char* file, int line,
                 const char* func, const char* fmt, ...)
#if defined(__clang__) || defined(__GNUC__)
                 __attribute__((format(printf,5,6)))
#endif
                 ;

void tx_log_set_file_path(const char* path);  // nullptr/"" -> "debugtraces.log"
void tx_log_enable_file(bool enabled);
bool tx_log_is_enabled();
void tx_log_flush();
void tx_log_enable_thread_id(bool enabled);
bool tx_log_is_thread_id_enabled();

#ifdef __cplusplus
}
#endif

// -------- RAII scope timer --------
#ifdef __cplusplus
struct TxScopeTimer {
  const char* name; const char* file; const char* func; int line;
  unsigned long long t0_ns;
  TxScopeTimer(const char* n, const char* f, int l, const char* fn) noexcept;
  ~TxScopeTimer() noexcept;
};
#endif

// -------- Macros (only these) --------
#if __cplusplus >= 202002L || (defined(__clang__) && __clang_major__ >= 9) || (defined(__GNUC__) && __GNUC__ >= 10)
// C++20 or recent compiler: use __VA_OPT__ for standards compliance
#define LOGF(fmt, ...) tx_log_emit(TxLogLevel::FATAL, __FILE__, __LINE__, __func__, fmt __VA_OPT__(,) __VA_ARGS__)
#define LOGE(fmt, ...) tx_log_emit(TxLogLevel::ERROR, __FILE__, __LINE__, __func__, fmt __VA_OPT__(,) __VA_ARGS__)
#define LOGW(fmt, ...) tx_log_emit(TxLogLevel::WARN,  __FILE__, __LINE__, __func__, fmt __VA_OPT__(,) __VA_ARGS__)
#define LOGI(fmt, ...) tx_log_emit(TxLogLevel::INFO,  __FILE__, __LINE__, __func__, fmt __VA_OPT__(,) __VA_ARGS__)
#define LOGD(fmt, ...) tx_log_emit(TxLogLevel::DBG, __FILE__, __LINE__, __func__, fmt __VA_OPT__(,) __VA_ARGS__)
#define TRACE(fmt, ...) tx_log_emit(TxLogLevel::DBG, __FILE__, __LINE__, __func__, fmt __VA_OPT__(,) __VA_ARGS__)
#else
// Fallback for older compilers: suppress the warning with pragma
#ifdef __clang__
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wgnu-zero-variadic-macro-arguments"
#endif
#define LOGF(fmt, ...) tx_log_emit(TxLogLevel::FATAL, __FILE__, __LINE__, __func__, fmt, ##__VA_ARGS__)
#define LOGE(fmt, ...) tx_log_emit(TxLogLevel::ERROR, __FILE__, __LINE__, __func__, fmt, ##__VA_ARGS__)
#define LOGW(fmt, ...) tx_log_emit(TxLogLevel::WARN,  __FILE__, __LINE__, __func__, fmt, ##__VA_ARGS__)
#define LOGI(fmt, ...) tx_log_emit(TxLogLevel::INFO,  __FILE__, __LINE__, __func__, fmt, ##__VA_ARGS__)
#define LOGD(fmt, ...) tx_log_emit(TxLogLevel::DBG, __FILE__, __LINE__, __func__, fmt, ##__VA_ARGS__)
#define TRACE(fmt, ...) tx_log_emit(TxLogLevel::DBG, __FILE__, __LINE__, __func__, fmt, ##__VA_ARGS__)
#ifdef __clang__
#pragma clang diagnostic pop
#endif
#endif

#ifdef __cplusplus
#define LOGTIMER(name) ::TxScopeTimer _tx_scope_timer_##__LINE__{(name), __FILE__, __LINE__, __func__}
#else
#define LOGTIMER(name) do{}while(0)
#endif

// -------- LOG_TO_FILE("filename") --------
// Enables file sink; defaults to "debugtraces.log" if no filename provided.
inline void tx_log_to_file_helper(const char* filename = nullptr) {
    const char* path = filename ? filename : "debugtraces.log";
    tx_log_set_file_path(path);
    tx_log_enable_file(true);
    tx_log_emit(TxLogLevel::INFO, __FILE__, __LINE__, __func__, 
                "File logging enabled: %s", path);
}

#define LOG_TO_FILE(...) tx_log_to_file_helper(__VA_ARGS__)

// -------- Thread ID Control Macros --------
#define ENABLE_THREAD_ID_TRACING() tx_log_enable_thread_id(true)
#define DISABLE_THREAD_ID_TRACING() tx_log_enable_thread_id(false)

#else  // TX_TRACE_THIS_FILE == 0  →  per-file no-ops

enum class TxLogLevel : uint8_t { FATAL=0, ERROR=1, WARN=2, INFO=3, DBG=4 };

#ifdef __cplusplus
extern "C" {
#endif

// Function declarations - implementations provided in Logger.cpp
void tx_log_emit(TxLogLevel, const char*, int, const char*, const char*, ...);
void tx_log_set_file_path(const char*);
void tx_log_enable_file(bool);
void tx_log_enable_thread_id(bool);
bool tx_log_is_thread_id_enabled();
bool tx_log_is_enabled();
void tx_log_flush();

#ifdef __cplusplus
}
#endif



#define LOGF(...)   do{}while(0)
#define LOGE(...)   do{}while(0)
#define LOGW(...)   do{}while(0)
#define LOGI(...)   do{}while(0)
#define LOGD(...)   do{}while(0)
#define TRACE(...)  do{}while(0)
#define LOGTIMER(...) do{}while(0)
#define LOG_TO_FILE(...) do{}while(0)
#define ENABLE_THREAD_ID_TRACING() do{}while(0)
#define DISABLE_THREAD_ID_TRACING() do{}while(0)

inline void tx_log_to_file_helper(const char* filename = nullptr) { (void)filename; }

// Stub TxScopeTimer for disabled tracing
#ifdef __cplusplus
struct TxScopeTimer {
    TxScopeTimer(const char*, const char*, int, const char*) noexcept;
    ~TxScopeTimer() noexcept;
};
#endif

// Expose API as no-ops

#endif  // TX_TRACE_THIS_FILE
