#pragma once
/**
 * DebugTracesLib.h — portable logging helpers with TRACE and timing
 *
 * Per-file controls (define BEFORE including this header in a .cpp):
 *   #define TX_TRACE_THIS_FILE 1   // enable TRACE/TRACES for this TU (debug builds)
 *   #define TX_MEASURE_TIME    1   // enable time-measurement helpers (debug builds)
 *   #define TX_LOG_LEVEL_FILE  TX_LVL_DEBUG  // optional compile-out level for this TU
 */

#include <cassert>
#include <cstddef>
#include <cstring> // strrchr


 // declares TX_Logger (runtime logger) and LogLevel (runtime enum)

/* ---------- Basename helper (portable) ---------- */
#ifndef TX_FILENAME
  #define TX_FILENAME \
    ( (strrchr(__FILE__, '\\') ? strrchr(__FILE__, '\\') + 1 : \
       (strrchr(__FILE__, '/') ? strrchr(__FILE__, '/') + 1 : __FILE__)) )
#endif

/* ---------- Assert (plus compatibility with ASSERT) ---------- */
#if defined(_MSC_VER) && !defined(NDEBUG)
  #define TX_ASSERT(x) \
    do { \
      __pragma(warning(push)) \
      __pragma(warning(disable:4127)) \
      if(!(x)){ TX_Logf(TX_LogLevel::Error, TX_FILENAME, __LINE__, __func__, "ASSERT(false)"); } \
      assert(x); \
      __pragma(warning(pop)) \
    } while(0)
#else
  #define TX_ASSERT(x) assert((x))
#endif
#ifndef ASSERT
  #define ASSERT(x) TX_ASSERT(x)  /* compatibility for existing sources */
#endif

/* =========================================================================
 *  Compile-time log-level gating for macros (must use integer macros in #if)
 * =========================================================================
 * Runtime API uses enum class LogLevel, but the preprocessor needs integers.
 */
/* integer levels for preprocessor */
#define TX_LVL_DEBUG 0
#define TX_LVL_INFO  1
#define TX_LVL_WARN  2
#define TX_LVL_ERROR 3

#ifndef TX_LOG_LEVEL_GLOBAL
  #define TX_LOG_LEVEL_GLOBAL TX_LVL_INFO
#endif
#ifndef TX_LOG_LEVEL_FILE
  #define TX_LOG_LEVEL_FILE TX_LOG_LEVEL_GLOBAL
#endif
#define TX__MINLV (TX_LOG_LEVEL_FILE)

/* compile-out by level (no ##__VA_ARGS__) */
#if TX__MINLV <= TX_LVL_DEBUG
  #define TX_LOG_DEBUG(...) TX_Logger::getPtr()->writeLine(LogLevel::LL_DEBUG, TX_FILENAME, __LINE__, __VA_ARGS__)
#else
  #define TX_LOG_DEBUG(...) (void)0
#endif
#if TX__MINLV <= TX_LVL_INFO
  #define TX_LOG_INFO(...)  TX_Logger::getPtr()->writeLine(LogLevel::LL_INFO,  TX_FILENAME, __LINE__, __VA_ARGS__)
#else
  #define TX_LOG_INFO(...)  (void)0
#endif
#if TX__MINLV <= TX_LVL_WARN
  #define TX_LOG_WARN(...)  TX_Logger::getPtr()->writeLine(LogLevel::LL_WARN,  TX_FILENAME, __LINE__, __VA_ARGS__)
#else
  #define TX_LOG_WARN(...)  (void)0
#endif
#if TX__MINLV <= TX_LVL_ERROR
  #define TX_LOG_ERROR(...) TX_Logger::getPtr()->writeLine(LogLevel::LL_ERROR, TX_FILENAME, __LINE__, __VA_ARGS__)
#else
  #define TX_LOG_ERROR(...) (void)0
#endif

/* TRACE always logs when enabled; banner is opt-in to avoid warnings */
#if TX_TRACE_THIS_FILE
#include "Logger.h"  
  #if TX_TRACE_BANNER
    #if defined(_MSC_VER)
      #pragma message("Tracing enabled for: " __FILE__)
    #elif defined(__clang__) || defined(__GNUC__)
      #pragma message "Tracing enabled for: " __FILE__
    #endif
  #endif
  #define TRACE(...)  TX_Logger::getPtr()->writeLine(LogLevel::LL_DEBUG, TX_FILENAME, __LINE__, __VA_ARGS__)
  #define TRACES(...) TRACE(__VA_ARGS__)
#else
  #define TRACE(...)  (void)0
  #define TRACES(...) (void)0
#endif



/* ---------- Portable time measurement helpers ---------- */
#if !defined(NDEBUG) && TX_MEASURE_TIME
  #include <chrono>
  #define TX__TIME_VARS(prefix) \
    std::chrono::high_resolution_clock::time_point prefix##_start = std::chrono::high_resolution_clock::now(); \
    std::chrono::high_resolution_clock::time_point prefix##_end;

  /* Start & print single-shot in same scope */
  #define TX_START_TIME_MEASUREMENT() TX__TIME_VARS(tx_tm)
  #define TX_PRINT_TIME_MEASUREMENT() \
    do{ tx_tm_end = std::chrono::high_resolution_clock::now(); \
        auto us = std::chrono::duration_cast<std::chrono::microseconds>(tx_tm_end - tx_tm_start).count(); \
        TX_LOG_DEBUG("Time Elapsed: %lld us", static_cast<long long>(us)); }while(0)

  /* Named contexts */
  #define TX_START_TIME_MEAS_CTX(ctx) TX__TIME_VARS(ctx)
  #define TX_PRINT_TIME_MEAS_CTX(ctx) \
    do{ ctx##_end = std::chrono::high_resolution_clock::now(); \
        auto us = std::chrono::duration_cast<std::chrono::microseconds>(ctx##_end - ctx##_start).count(); \
        TX_LOG_DEBUG("Time Elapsed (%s): %lld us", #ctx, static_cast<long long>(us)); }while(0)
#else
  #define TX_START_TIME_MEASUREMENT()   do{}while(0)
  #define TX_PRINT_TIME_MEASUREMENT()   do{}while(0)
  #define TX_START_TIME_MEAS_CTX(ctx)   do{}while(0)
  #define TX_PRINT_TIME_MEAS_CTX(ctx)   do{}while(0)
#endif

/* ---------- Misc ---------- */
#define UNUSED_PARAM(x) ((void)(x))
