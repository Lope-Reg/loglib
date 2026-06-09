#pragma once
#include <string>
#include <vector>
#include <memory>
#include <sstream>
#include <mutex>
#include "LogLevel.h"
#include "LogEvent.h"
#include "LogFormatter.h"
#include "LogAppender.h"

namespace loglib {

class Logger : public std::enable_shared_from_this<Logger> {
public:
    using ptr = std::shared_ptr<Logger>;

    explicit Logger(const std::string& name = "root");

    // 核心接口
    void log(LogLevel level, const std::string& file, int line, const std::string& msg);

    // 便捷方法
    void debug(const std::string& file, int line, const std::string& msg);
    void info(const std::string& file, int line, const std::string& msg);
    void warn(const std::string& file, int line, const std::string& msg);
    void error(const std::string& file, int line, const std::string& msg);
    void fatal(const std::string& file, int line, const std::string& msg);

    // 流式日志 —— C++11: RAII 代理对象
    class LogStream {
    public:
        LogStream(Logger::ptr logger, LogLevel level, const char* file, int line);
        ~LogStream();  // 析构时提交日志

        // C++11: 模板
        template<typename T>
        LogStream& operator<<(const T& value) {
            m_ss << value;
            return *this;
        }

    private:
        Logger::ptr m_logger;
        LogLevel m_level;
        const char* m_file;
        int m_line;
        std::stringstream m_ss;
    };

    LogStream stream(LogLevel level, const char* file, int line);

    // 配置接口
    void addAppender(LogAppender::ptr appender);
    void removeAppender(LogAppender::ptr appender);
    void setFormatter(LogFormatter::ptr formatter);
    void setLevel(LogLevel level) { m_level = level; }
    LogLevel getLevel() const { return m_level; }
    const std::string& getName() const { return m_name; }

private:
    std::string m_name;
    LogLevel m_level = LogLevel::DEBUG;
    LogFormatter::ptr m_formatter;
    std::vector<LogAppender::ptr> m_appenders;  // STL: vector
    mutable std::mutex m_mutex;
};

} // namespace loglib
