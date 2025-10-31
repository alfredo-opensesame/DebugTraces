#pragma once

#include <memory>
#include <sstream>
#include <cstring>

#include "Writer.h"

#define __FILENAME__ (strrchr(__FILE__, \'\\') ? strrchr(__FILE__, \'\\') + 1 : (strrchr(__FILE__, '/') ? strrchr(__FILE__, '/') + 1 : __FILE__))

#define TRACE_ERROR(message)   TX_Logger::getPtr()->writeError(message);
#define TRACE_WARNING(message) TX_Logger::getPtr()->writeWarning(message);
#define TRACE_INFO(message)    TX_Logger::getPtr()->writeInfo(message);

#define LogFatal(format, ...)         TX_Logger::getPtr()->writeLine(LogLevel::LL_FATAL, __FILENAME__, __LINE__, __VA_ARGS__)
#define LogFatalStatic(format, ...)   TX_Logger::getPtr()->writeLine(LogLevel::LL_FATAL, __FILENAME__, __LINE__, __VA_ARGS__)

#define LogError(format, ...)         TX_Logger::getPtr()->writeLine(LogLevel::LL_ERROR, __FILENAME__, __LINE__, __VA_ARGS__)
#define LogErrorStatic(format, ...)   TX_Logger::getPtr()->writeLine(LogLevel::LL_ERROR, __FILENAME__, __LINE__, __VA_ARGS__)

#define LogWarn(format, ...)          TX_Logger::getPtr()->writeLine(LogLevel::LL_WARN , __FILENAME__, __LINE__, __VA_ARGS__)
#define LogWarnStatic(format, ...)    TX_Logger::getPtr()->writeLine(LogLevel::LL_WARN , __FILENAME__, __LINE__, __VA_ARGS__)

#define LogInfo(format, ...)          TX_Logger::getPtr()->writeLine(LogLevel::LL_INFO , __FILENAME__, __LINE__, __VA_ARGS__)
#define LogInfoStatic(format, ...)    TX_Logger::getPtr()->writeLine(LogLevel::LL_INFO , __FILENAME__, __LINE__, __VA_ARGS__)

#ifdef _DEBUG
#define LogDebug(format, ...)         TX_Logger::getPtr()->writeLine(LogLevel::LL_DEBUG, __FILENAME__, __LINE__, __VA_ARGS__)
#define LogDebugStatic(format, ...)   TX_Logger::getPtr()->writeLine(LogLevel::LL_DEBUG, __FILENAME__, __LINE__, __VA_ARGS__)

#define LogTrace(format, ...)         TX_Logger::getPtr()->writeLine(LogLevel::LL_TRACE, __FILENAME__, __LINE__, __VA_ARGS__)
#define LogTraceStatic(format, ...)   TX_Logger::getPtr()->writeLine(LogLevel::LL_TRACE, __FILENAME__, __LINE__, __VA_ARGS__)
#else
#define LogDebug(format, ...)
#define LogDebugStatic(format, ...)

#define LogTrace(format, ...)
#define LogTraceStatic(format, ...)
#endif

#define message_size 1024

enum class LogLevel
{
    LL_FATAL,
    LL_ERROR,
    LL_WARN,
    LL_INFO,
    LL_DEBUG,
    LL_TRACE
};

class TX_Logger
{
public:

    ~TX_Logger();

    static TX_Logger& instance() noexcept;
    static TX_Logger* getPtr() noexcept { return &instance(); }
    static void startInstance() noexcept { (void)instance(); }
    static void releaseInstance() noexcept { /* no-op; immortal singleton */ }

    void setLogLevel(const LogLevel & level);

    void writeInfo(const std::string & message);
    void writeWarning(const std::string & message);
    void writeError(const std::string & message);

    void writeLine(LogLevel logLevel, const char* file, const int line, const char* format, ...);

    void logIntoFile(const std::string & path, const size_t numLines = 0);
    void logIntoConsole();

#ifdef GTEST
    void resetWriter();
#endif

private:

    TX_Logger() = default;
    TX_Logger(const TX_Logger& root) = delete;
    TX_Logger & operator = (const TX_Logger&) = delete;

    void  writeMessage(const std::string& message);
    void  writeIDEDebugString(const std::string & message);
    const std::string getLogString(LogLevel logLevel, const char* file, const int line, const std::string & message);

            
    std::unique_ptr <Writer> m_writer;

    LogLevel                 m_logLevel         = LogLevel::LL_TRACE;
    bool                     m_isConsoleLogging = true;
};

class TraceInfo
    : public std::ostringstream
{
public:
    ~TraceInfo()
    {
        this->flush();
        TX_Logger::getPtr()->writeInfo(this->str());
    }
};

class TraceWarning
    : public std::ostringstream
{
public:
    ~TraceWarning()
    {
        this->flush();
        TX_Logger::getPtr()->writeWarning(this->str());
    }
};

class TraceError
    : public std::ostringstream
{
public:

    ~TraceError()
    {
        this->flush();
        TX_Logger::getPtr()->writeError(this->str());
    }
};