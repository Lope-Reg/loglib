#pragma once
#include <string>
#include <chrono>
#include <memory>
#include <thread>
#include "LogLevel.h"

namespace loglib {

// 日志事件: 封装一条日志的所有上下文信息
class LogEvent {
public:
    using ptr = std::shared_ptr<LogEvent>;

    LogEvent(const char* file, int line, LogLevel level,
             const std::string& loggerName, const std::string& content)
        : m_file(file)
        , m_line(line)
        , m_level(level)
        , m_loggerName(loggerName)
        , m_content(content)
        , m_time(std::chrono::system_clock::now())
        , m_threadId(std::hash<std::thread::id>{}(std::this_thread::get_id()))
    {}

    // 封装: public 接口访问 private 数据
    const char* getFile() const { return m_file; }
    int getLine() const { return m_line; }
    LogLevel getLevel() const { return m_level; }
    const std::string& getLoggerName() const { return m_loggerName; }
    const std::string& getContent() const { return m_content; }
    uint64_t getThreadId() const { return m_threadId; }
    std::chrono::system_clock::time_point getTime() const { return m_time; }

private:
    const char* m_file;
    int m_line;
    LogLevel m_level;
    std::string m_loggerName;
    std::string m_content;
    uint64_t m_threadId;
    std::chrono::system_clock::time_point m_time;
};

} // namespace loglib
