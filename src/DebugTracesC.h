#pragma once

/**
 * @file DebugTracesC.h
 * @brief C interface for the DebugTraces logging library
 * 
 * This header provides a pure C interface to the thread-safe logging
 * functionality, enabling use from C projects while maintaining full
 * compatibility with the C++ implementation.
 * 
 * Thread Safety: All functions are thread-safe and can be called
 * concurrently from multiple threads.
 */

#ifdef __cplusplus
extern "C" {
#endif

#include <stddef.h>

/* Log levels for C interface */
typedef enum {
    TX_LOGLEVEL_FATAL = 0,
    TX_LOGLEVEL_ERROR = 1,
    TX_LOGLEVEL_WARN = 2,
    TX_LOGLEVEL_INFO = 3,
    TX_LOGLEVEL_DEBUG = 4,
    TX_LOGLEVEL_TRACE = 5
} tx_log_level_t;

/**
 * @brief Initialize the logger system
 * 
 * Call this once at program startup. Thread-safe to call multiple times.
 */
void tx_logger_init(void);

/**
 * @brief Cleanup the logger system
 * 
 * Call this at program shutdown. Optional since cleanup is automatic.
 */
void tx_logger_cleanup(void);

/**
 * @brief Write a formatted log message
 * 
 * @param log_level Log level (use tx_log_level_t enum values)
 * @param file Source file name (__FILE__ macro)
 * @param line Source line number (__LINE__ macro)
 * @param format Printf-style format string
 * @param ... Format arguments
 * 
 * Thread-safe: Yes
 */
void tx_logger_write_line_c(int log_level, const char* file, int line, const char* format, ...);

/**
 * @brief Set the minimum log level
 * 
 * @param level Minimum level to log (use tx_log_level_t enum values)
 * 
 * Thread-safe: Yes
 */
void tx_logger_set_level_c(int level);

/**
 * @brief Enable console logging
 * 
 * Thread-safe: Yes
 */
void tx_logger_log_to_console_c(void);

/**
 * @brief Enable file logging
 * 
 * @param path File path for log output
 * @param max_lines Maximum lines before rotation (0 = no limit)
 * 
 * Thread-safe: Yes
 */
void tx_logger_log_to_file_c(const char* path, size_t max_lines);

/* Convenience macros for C projects */
#define TX_LOG_FATAL(...) tx_logger_write_line_c(TX_LOGLEVEL_FATAL, __FILE__, __LINE__, __VA_ARGS__)
#define TX_LOG_ERROR(...) tx_logger_write_line_c(TX_LOGLEVEL_ERROR, __FILE__, __LINE__, __VA_ARGS__)
#define TX_LOG_WARN(...)  tx_logger_write_line_c(TX_LOGLEVEL_WARN,  __FILE__, __LINE__, __VA_ARGS__)
#define TX_LOG_INFO(...)  tx_logger_write_line_c(TX_LOGLEVEL_INFO,  __FILE__, __LINE__, __VA_ARGS__)

#ifdef NDEBUG
  #define TX_LOG_DEBUG(...) ((void)0)
  #define TX_LOG_TRACE(...) ((void)0)
#else
  #define TX_LOG_DEBUG(...) tx_logger_write_line_c(TX_LOGLEVEL_DEBUG, __FILE__, __LINE__, __VA_ARGS__)
  #define TX_LOG_TRACE(...) tx_logger_write_line_c(TX_LOGLEVEL_TRACE, __FILE__, __LINE__, __VA_ARGS__)
#endif

#ifdef __cplusplus
}
#endif