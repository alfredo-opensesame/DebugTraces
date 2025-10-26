// TX_Logger.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include "Logger.h"
#include "Writer.h"
#include <iostream>
#include <sstream>
#include <ctime>

#if defined _MSC_VER
#include <Windows.h>
#elif defined __APPLE__
#include <syslog.h>
#endif
#include <thread>
#include <iomanip>
#include <cstdarg>
#include <atomic>

namespace {
  std::atomic<bool> g_txlogger_alive{false};
}

TX_Logger& TX_Logger::instance() noexcept {
    static TX_Logger s;                 // Meyers singleton
    g_txlogger_alive.store(true, std::memory_order_release);
    return s;
}

TX_Logger::~TX_Logger() {
    // Mark as not alive so late logs during static teardown are dropped
    g_txlogger_alive.store(false, std::memory_order_release);
}

void TX_Logger::writeError(const std::string& message)
{
    if (m_logLevel >= LogLevel::LL_ERROR)
        writeMessage(getLogString(LogLevel::LL_ERROR, nullptr, 0, message));
}

void TX_Logger::writeWarning(const std::string& message)
{
    if (m_logLevel >= LogLevel::LL_WARN)
        writeMessage(getLogString(LogLevel::LL_WARN, nullptr, 0, message));
}

void TX_Logger::writeInfo(const std::string& message)
{
    if (m_logLevel >= LogLevel::LL_INFO)
        writeMessage(getLogString(LogLevel::LL_INFO, nullptr, 0, message));
}

void TX_Logger::writeLine(LogLevel logLevel, const char* file, const int line, const char* format, ...)
{
    if (!g_txlogger_alive.load(std::memory_order_acquire)) return;

    if (m_logLevel >= logLevel)
    {
        char message[message_size];

        va_list args;
        va_start(args, format);
        vsnprintf(message, message_size, format, args);
        va_end(args);

        writeMessage(getLogString(logLevel, file, line, message));
    }
}

void TX_Logger::setLogLevel(const LogLevel & level)
{
    m_logLevel = level;
}

const std::string TX_Logger::getLogString(LogLevel logLevel, const char* file, const int line, const std::string & message)
{
    std::ostringstream oss;
    char timedisplay[100];
    struct tm buf;

#if defined _MSC_VER
    auto now = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
    localtime_s(&buf, &now);
#else
    time_t now;
    time(&now);
    localtime_r(&now, &buf);
#endif

    std::strftime(timedisplay, sizeof(timedisplay), "%Y.%m.%d %H:%M:%S", &buf);
    oss << timedisplay;

    if (file != nullptr)
        oss << "(" << file << ":" << line << ")";

    oss << " [" << std::setw(6) << std::setfill('0') << std::this_thread::get_id() << "]";

    switch (logLevel)
    {
    case LogLevel::LL_INFO:
        oss << " - INFO:  ";
        break;
    case LogLevel::LL_WARN:
        oss << " - WRN:   ";
        break;
    case LogLevel::LL_ERROR:
        oss << " - ERR:   ";
        break;
    case LogLevel::LL_FATAL:
        oss << " - FATAL:  ";
        break;
	default:
		break;
    }

    oss << message << std::endl;
    return std::move(oss).str();
}

void TX_Logger::writeMessage(const std::string& message)
{
    if (!g_txlogger_alive.load(std::memory_order_acquire)) return;
    if (m_writer != nullptr)
        m_writer->appendString(message);

    if (m_isConsoleLogging)
        writeIDEDebugString(message);
}

void TX_Logger::writeIDEDebugString(const std::string& message)
{
#if defined _MSC_VER
    OutputDebugStringA(message.c_str());
#elif defined __APPLE__
    // Pick ONE sink. Define TX_LOG_TO_SYSLOG to send via syslog; otherwise stderr.
    #if defined(TX_LOG_TO_SYSLOG)
      syslog(LOG_ERR, "Logger: %s", message.c_str());
    #else
      std::cerr << message;
    #endif
#else
    std::cerr << message;
#endif
}

void TX_Logger::logIntoFile(const std::string & path, const size_t numLines)
{
    m_writer = std::make_unique<Writer>(path, numLines);
}

void TX_Logger::logIntoConsole()
{
    m_isConsoleLogging = true;
}

#ifdef GTEST

void TX_Logger::resetWriter()
{
    m_writer = nullptr;
}

#endif
