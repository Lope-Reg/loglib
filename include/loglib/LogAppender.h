#pragma once
#include <string>
#include <memory>
#include <mutex>
#include <fstream>
#include "LogLevel.h"

namespace loglib {

// 抽象基类 —— 多态接口
class LogAppender {
public:
    using ptr = std::shared_ptr<LogAppender>;

    virtual ~LogAppender() = default;

    // 纯虚函数: 子类必须实现
    virtual void log(LogLevel level, const std::string& msg) = 0;

    void setLevel(LogLevel level) { m_level = level; }
    LogLevel getLevel() const { return m_level; }

protected:
    LogLevel m_level = LogLevel::DEBUG;
    mutable std::mutex m_mutex;
};

// 具体输出器 1: 控制台 (带颜色)
class StdoutAppender : public LogAppender {
public:
    void log(LogLevel level, const std::string& msg) override;

private:
    std::string colorize(LogLevel level, const std::string& msg);
};

// 具体输出器 2: 文件
class FileAppender : public LogAppender {
public:
    explicit FileAppender(const std::string& filename);
    ~FileAppender();

    void log(LogLevel level, const std::string& msg) override;

    // C++11: 禁用拷贝，允许移动
    FileAppender(const FileAppender&) = delete;
    FileAppender& operator=(const FileAppender&) = delete;
    FileAppender(FileAppender&&) noexcept = default;
    FileAppender& operator=(FileAppender&&) noexcept = default;

private:
    std::string m_filename;
    std::ofstream m_ofstream;
};

// 具体输出器 3: 滚动文件 (按大小切割)
class RollingFileAppender : public LogAppender {
public:
    RollingFileAppender(const std::string& basename,
                        size_t maxSize = 10 * 1024 * 1024);

    void log(LogLevel level, const std::string& msg) override;

private:
    void roll();

    std::string m_basename;
    size_t m_maxSize;
    size_t m_currentSize = 0;
    int m_fileIndex = 0;
    std::ofstream m_ofstream;
};

} // namespace loglib
