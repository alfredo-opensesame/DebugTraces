#pragma once

#include <thread>
#include <mutex>
#include <deque>
#include <atomic>
#include <condition_variable>
#include <string>
#include <fstream>

class Writer
{
public:
    explicit Writer(const std::string & path, const size_t numLines = 0);
    ~Writer();

    void appendString(const std::string & message);

private:
    constexpr inline static size_t cleanFilePer = 70;

    void loop();
    size_t correctFileSize(std::fstream & stream);

    std::string              m_filePath;
    size_t                   m_maxNumLines;

    std::mutex               m_mutex;
    std::condition_variable  m_cv;
    std::deque <std::string> m_messages;
    bool                     m_isDone;
    std::thread	             m_worker;
};