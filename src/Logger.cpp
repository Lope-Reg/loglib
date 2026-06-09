#include "loglib/Logger.h"
#include "loglib/PatternFormatter.h"
#include <algorithm>

namespace loglib {

Logger::Logger(const std::string& name)
    : m_name(name)
    , m_formatter(std::make_shared<PatternFormatter>()) {
}

void Logger::log(LogLevel level, const std::string& file, int line, const std::string& msg) {
    if (level < m_level) return;

    auto event = std::make_shared<LogEvent>(
        file.c_str(), line, level, m_name, msg);

    std::string formatted = m_formatter->format(event);

    // 多态调用: 分发到所有 Appender
    std::lock_guard<std::mutex> lock(m_mutex);
    for (auto& appender : m_appenders) {
        appender->log(level, formatted);
    }
}

void Logger::debug(const std::string& file, int line, const std::string& msg) {
    log(LogLevel::DEBUG, file, line, msg);
}
void Logger::info(const std::string& file, int line, const std::string& msg) {
    log(LogLevel::INFO, file, line, msg);
}
void Logger::warn(const std::string& file, int line, const std::string& msg) {
    log(LogLevel::WARN, file, line, msg);
}
void Logger::error(const std::string& file, int line, const std::string& msg) {
    log(LogLevel::ERROR, file, line, msg);
}
void Logger::fatal(const std::string& file, int line, const std::string& msg) {
    log(LogLevel::FATAL, file, line, msg);
}

// ---- LogStream ----
Logger::LogStream::LogStream(Logger::ptr logger, LogLevel level,
                              const char* file, int line)
    : m_logger(std::move(logger))
    , m_level(level)
    , m_file(file)
    , m_line(line) {}

Logger::LogStream::~LogStream() {
    // RAII: 析构时自动提交日志
    m_logger->log(m_level, m_file, m_line, m_ss.str());
}

Logger::LogStream Logger::stream(LogLevel level, const char* file, int line) {
    return LogStream(shared_from_this(), level, file, line);
}

void Logger::addAppender(LogAppender::ptr appender) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_appenders.push_back(std::move(appender));
}

void Logger::removeAppender(LogAppender::ptr appender) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_appenders.erase(
        std::remove(m_appenders.begin(), m_appenders.end(), appender),
        m_appenders.end());
}

void Logger::setFormatter(LogFormatter::ptr formatter) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_formatter = std::move(formatter);
}

} // namespace loglib
