#include "Writer.h"

#include <algorithm>
#include <fstream>
#include <string>
#include <vector>
#if defined _MSC_VER
#include <Windows.h>
#endif

Writer::Writer(const std::string& path, size_t numLines)
    : m_filePath(path)
    , m_maxNumLines(numLines)
    , m_isDone(false)
    , m_worker([this]{ loop(); })
{
}

Writer::~Writer()
{
    {
        std::lock_guard <std::mutex> locker(m_mutex);
        m_isDone = true;
    }

    m_cv.notify_one();
    m_worker.join();
}

void Writer::loop()
{
    std::deque <std::string> buffer;

    std::fstream stream;
    stream.open(m_filePath, std::ios::in | std::ios::out | std::ios::app);
    if (stream.is_open())
    {
        std::istreambuf_iterator<char> begin(stream), end;
        size_t numLines = static_cast<size_t>(std::count(begin, end, char('\n')));

        while (true)
        {
            {
                std::unique_lock <std::mutex> locker(m_mutex);
                m_cv.wait(locker, [this]  { return !m_messages.empty() || m_isDone; });

                while (!m_messages.empty())
                {
                    buffer.push_back(std::move(m_messages.front()));
                    m_messages.pop_front();
                }
            }

            while (!buffer.empty())
            {
                stream << std::move(buffer.front());
                buffer.pop_front();
                ++numLines;

                if (m_maxNumLines > 0 && m_maxNumLines < numLines)
                    numLines = correctFileSize(stream);
            }

            stream.flush();
            std::unique_lock <std::mutex> locker(m_mutex);
            if (m_isDone && m_messages.empty())
                break;
        }

        stream.flush();
        stream.close();
    }
}

size_t Writer::correctFileSize(std::fstream & stream)
{
    std::string str;
    std::vector <std::string> fileLines;
    size_t counter = 0;

    stream.seekg(0);
    stream.flush();

    size_t linesToKeep = m_maxNumLines * cleanFilePer / 100;
    while (std::getline(stream, str))
        if (counter++ >= (m_maxNumLines - linesToKeep))
            fileLines.push_back(std::move(str));

    stream.close();
    std::remove(m_filePath.c_str());
    stream.open(m_filePath, std::ios::in | std::ios::out | std::ios::app);

    counter = 0;
    for (auto & line : fileLines)
    {
        stream << std::move(line) << std::endl;
        ++counter;
    }

    return counter;
}

void Writer::appendString(const std::string & message)
{
    std::lock_guard <std::mutex> lock(m_mutex);
    m_messages.push_back(message);

    m_cv.notify_one();
}
