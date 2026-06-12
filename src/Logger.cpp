#include "_internal.h"
#include <algorithm>

namespace loglib {

// ---- Logger ----
Logger::Logger(const std::string& name)
    : m_impl(std::make_unique<Impl>()) {
    m_impl->name = name;
    m_impl->formatter = std::make_shared<PatternFormatter>();
}

Logger::~Logger() = default;

void Logger::log(LogLevel level, const std::string& file, int line, const std::string& msg) {
    if (level < m_impl->level) return;

    auto event = std::make_shared<LogEvent>(file.c_str(), line, level, m_impl->name, msg);
    std::string formatted = m_impl->formatter->format(event);

    std::lock_guard<std::mutex> lock(m_impl->mutex);
    for (auto& appender : m_impl->appenders) {
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

void Logger::addAppender(LogAppender::ptr appender) {
    std::lock_guard<std::mutex> lock(m_impl->mutex);
    m_impl->appenders.push_back(std::move(appender));
}

void Logger::removeAppender(LogAppender::ptr appender) {
    std::lock_guard<std::mutex> lock(m_impl->mutex);
    m_impl->appenders.erase(
        std::remove(m_impl->appenders.begin(), m_impl->appenders.end(), appender),
        m_impl->appenders.end());
}

void Logger::setFormatter(LogFormatter::ptr formatter) {
    std::lock_guard<std::mutex> lock(m_impl->mutex);
    m_impl->formatter = std::move(formatter);
}

void Logger::setLevel(LogLevel level) { m_impl->level = level; }
LogLevel Logger::getLevel() const { return m_impl->level; }
const std::string& Logger::getName() const { return m_impl->name; }

// ---- LogStream ----
Logger::LogStream::LogStream(Logger::ptr logger, LogLevel level,
                              const char* file, int line)
    : m_impl(std::make_unique<Impl>()) {
    m_impl->logger = std::move(logger);
    m_impl->level = level;
    m_impl->file = file;
    m_impl->line = line;
}

Logger::LogStream::~LogStream() {
    m_impl->logger->log(m_impl->level, m_impl->file, m_impl->line, m_impl->ss.str());
}

void Logger::LogStream::append(const std::string& v) { m_impl->ss << v; }
void Logger::LogStream::append(const char* v) { m_impl->ss << v; }
void Logger::LogStream::append(int v) { m_impl->ss << v; }
void Logger::LogStream::append(long v) { m_impl->ss << v; }
void Logger::LogStream::append(long long v) { m_impl->ss << v; }
void Logger::LogStream::append(unsigned int v) { m_impl->ss << v; }
void Logger::LogStream::append(unsigned long v) { m_impl->ss << v; }
void Logger::LogStream::append(unsigned long long v) { m_impl->ss << v; }
void Logger::LogStream::append(float v) { m_impl->ss << v; }
void Logger::LogStream::append(double v) { m_impl->ss << v; }
void Logger::LogStream::append(bool v) { m_impl->ss << v; }

Logger::LogStream Logger::stream(LogLevel level, const char* file, int line) {
    return LogStream(shared_from_this(), level, file, line);
}

} // namespace loglib
