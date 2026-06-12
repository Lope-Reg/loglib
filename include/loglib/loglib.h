// ============================================================================
// LogLib —— C++ 日志系统
// 用法: #include "loglib.h"  编译: -lloglib -lpthread
// ============================================================================
#pragma once

#include <string>
#include <memory>

namespace loglib {

// ======================== 日志级别 ========================
enum class LogLevel {
    DEBUG = 0,
    INFO  = 1,
    WARN  = 2,
    ERROR = 3,
    FATAL = 4
};

const char* toString(LogLevel level);

// ======================== 日志事件 ========================
class LogEvent {
public:
    using ptr = std::shared_ptr<LogEvent>;

    LogEvent(const char* file, int line, LogLevel level,
             const std::string& loggerName, const std::string& content);
    ~LogEvent();

    const char* getFile() const;
    int getLine() const;
    LogLevel getLevel() const;
    const std::string& getLoggerName() const;
    const std::string& getContent() const;
    uint64_t getThreadId() const;

private:
    struct Impl;
    std::unique_ptr<Impl> m_impl;
};

// ======================== 格式化器 (抽象基类) ========================
class LogFormatter {
public:
    using ptr = std::shared_ptr<LogFormatter>;

    virtual ~LogFormatter();
    virtual std::string format(LogEvent::ptr event) = 0;
};

// 内置格式化器: 支持 %d %t %p %f %l %m %n 等模式
class PatternFormatter : public LogFormatter {
public:
    explicit PatternFormatter(const std::string& pattern = "%d{%Y-%m-%d %H:%M:%S}%T[%p]%T%m%n");
    ~PatternFormatter();
    std::string format(LogEvent::ptr event) override;

private:
    struct Impl;
    std::unique_ptr<Impl> m_impl;
};

// ======================== 输出器 (抽象基类) ========================
class LogAppender {
public:
    using ptr = std::shared_ptr<LogAppender>;

    LogAppender();
    virtual ~LogAppender();
    virtual void log(LogLevel level, const std::string& msg) = 0;

    void setLevel(LogLevel level);
    LogLevel getLevel() const;

protected:
    LogLevel m_level;
};

// 控制台输出 (带颜色)
class StdoutAppender : public LogAppender {
public:
    StdoutAppender();
    ~StdoutAppender();
    void log(LogLevel level, const std::string& msg) override;

private:
    struct Impl;
    std::unique_ptr<Impl> m_impl;
};

// 文件输出
class FileAppender : public LogAppender {
public:
    explicit FileAppender(const std::string& filename);
    ~FileAppender();
    void log(LogLevel level, const std::string& msg) override;

    FileAppender(const FileAppender&) = delete;
    FileAppender& operator=(const FileAppender&) = delete;

private:
    struct Impl;
    std::unique_ptr<Impl> m_impl;
};

// 滚动文件输出 (按大小切割)
class RollingFileAppender : public LogAppender {
public:
    explicit RollingFileAppender(const std::string& basename,
                                 size_t maxSize = 10 * 1024 * 1024);
    ~RollingFileAppender();
    void log(LogLevel level, const std::string& msg) override;

private:
    struct Impl;
    std::unique_ptr<Impl> m_impl;
};

// 异步装饰器: 包装任意 Appender 使其异步写入
class AsyncAppender : public LogAppender {
public:
    explicit AsyncAppender(LogAppender::ptr inner);
    ~AsyncAppender();
    void log(LogLevel level, const std::string& msg) override;

private:
    struct Impl;
    std::unique_ptr<Impl> m_impl;
};

// ======================== 日志器 ========================
class Logger : public std::enable_shared_from_this<Logger> {
public:
    using ptr = std::shared_ptr<Logger>;

    explicit Logger(const std::string& name = "root");
    ~Logger();

    // 直接写日志
    void log(LogLevel level, const std::string& file, int line, const std::string& msg);
    void debug(const std::string& file, int line, const std::string& msg);
    void info(const std::string& file, int line, const std::string& msg);
    void warn(const std::string& file, int line, const std::string& msg);
    void error(const std::string& file, int line, const std::string& msg);
    void fatal(const std::string& file, int line, const std::string& msg);

    // 流式日志
    class LogStream {
    public:
        LogStream(Logger::ptr logger, LogLevel level, const char* file, int line);
        ~LogStream();

        template<typename T>
        LogStream& operator<<(const T& value) {
            append(value);
            return *this;
        }

    private:
        void append(const std::string& v);
        void append(const char* v);
        void append(int v);
        void append(long v);
        void append(long long v);
        void append(unsigned int v);
        void append(unsigned long v);
        void append(unsigned long long v);
        void append(float v);
        void append(double v);
        void append(bool v);

        struct Impl;
        std::unique_ptr<Impl> m_impl;
    };

    LogStream stream(LogLevel level, const char* file, int line);

    // 配置
    void addAppender(LogAppender::ptr appender);
    void removeAppender(LogAppender::ptr appender);
    void setFormatter(LogFormatter::ptr formatter);
    void setLevel(LogLevel level);
    LogLevel getLevel() const;
    const std::string& getName() const;

private:
    struct Impl;
    std::unique_ptr<Impl> m_impl;
};

// ======================== 日志管理器 (单例+工厂) ========================
class LoggerManager {
public:
    static LoggerManager& getInstance();

    Logger::ptr createLogger(const std::string& name, LogLevel level = LogLevel::DEBUG);
    Logger::ptr getLogger(const std::string& name);
    Logger::ptr getRoot();
    void init();

    LoggerManager(const LoggerManager&) = delete;
    LoggerManager& operator=(const LoggerManager&) = delete;

private:
    LoggerManager();
    ~LoggerManager();
    struct Impl;
    std::unique_ptr<Impl> m_impl;
};

// ======================== 便捷宏 ========================

#define LOG_LEVEL(logger, level, msg) \
    (logger)->log(level, __FILE__, __LINE__, msg)

#define LOG_DEBUG(logger, msg) LOG_LEVEL(logger, loglib::LogLevel::DEBUG, msg)
#define LOG_INFO(logger, msg)  LOG_LEVEL(logger, loglib::LogLevel::INFO,  msg)
#define LOG_WARN(logger, msg)  LOG_LEVEL(logger, loglib::LogLevel::WARN,  msg)
#define LOG_ERROR(logger, msg) LOG_LEVEL(logger, loglib::LogLevel::ERROR, msg)
#define LOG_FATAL(logger, msg) LOG_LEVEL(logger, loglib::LogLevel::FATAL, msg)

#define LOG_STREAM(logger, level) \
    (logger)->stream(level, __FILE__, __LINE__)

#define LOG_DEBUG_S(logger) LOG_STREAM(logger, loglib::LogLevel::DEBUG)
#define LOG_INFO_S(logger)  LOG_STREAM(logger, loglib::LogLevel::INFO)
#define LOG_WARN_S(logger)  LOG_STREAM(logger, loglib::LogLevel::WARN)
#define LOG_ERROR_S(logger) LOG_STREAM(logger, loglib::LogLevel::ERROR)
#define LOG_FATAL_S(logger) LOG_STREAM(logger, loglib::LogLevel::FATAL)

#define ROOT_LOG_DEBUG(msg) LOG_DEBUG(loglib::LoggerManager::getInstance().getRoot(), msg)
#define ROOT_LOG_INFO(msg)  LOG_INFO(loglib::LoggerManager::getInstance().getRoot(), msg)
#define ROOT_LOG_WARN(msg)  LOG_WARN(loglib::LoggerManager::getInstance().getRoot(), msg)
#define ROOT_LOG_ERROR(msg) LOG_ERROR(loglib::LoggerManager::getInstance().getRoot(), msg)
#define ROOT_LOG_FATAL(msg) LOG_FATAL(loglib::LoggerManager::getInstance().getRoot(), msg)

} // namespace loglib
