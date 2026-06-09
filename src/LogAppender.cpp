#include "loglib/LogAppender.h"
#include <iostream>
#include <iomanip>

namespace loglib {

// ---- StdoutAppender ----
std::string StdoutAppender::colorize(LogLevel level, const std::string& msg) {
    switch (level) {
        case LogLevel::DEBUG: return "\033[36m" + msg + "\033[0m"; // cyan
        case LogLevel::INFO:  return "\033[32m" + msg + "\033[0m"; // green
        case LogLevel::WARN:  return "\033[33m" + msg + "\033[0m"; // yellow
        case LogLevel::ERROR: return "\033[31m" + msg + "\033[0m"; // red
        case LogLevel::FATAL: return "\033[35m" + msg + "\033[0m"; // magenta
        default: return msg;
    }
}

void StdoutAppender::log(LogLevel level, const std::string& msg) {
    if (level < m_level) return;
    std::lock_guard<std::mutex> lock(m_mutex);  // C++11: RAII 锁
    std::cout << colorize(level, msg);
}

// ---- FileAppender ----
FileAppender::FileAppender(const std::string& filename)
    : m_filename(filename) {
    m_ofstream.open(filename, std::ios::app);
}

FileAppender::~FileAppender() {
    if (m_ofstream.is_open()) {
        m_ofstream.close();
    }
}

void FileAppender::log(LogLevel level, const std::string& msg) {
    if (level < m_level) return;
    std::lock_guard<std::mutex> lock(m_mutex);
    m_ofstream << msg;
    m_ofstream.flush();
}

// ---- RollingFileAppender ----
RollingFileAppender::RollingFileAppender(const std::string& basename, size_t maxSize)
    : m_basename(basename), m_maxSize(maxSize) {
    roll();
}

void RollingFileAppender::log(LogLevel level, const std::string& msg) {
    if (level < m_level) return;
    std::lock_guard<std::mutex> lock(m_mutex);

    m_currentSize += msg.size();
    if (m_currentSize >= m_maxSize) {
        roll();
    }

    m_ofstream << msg;
    m_ofstream.flush();
}

void RollingFileAppender::roll() {
    if (m_ofstream.is_open()) {
        m_ofstream.close();
    }
    std::string filename = m_basename + "." + std::to_string(m_fileIndex++) + ".log";
    m_ofstream.open(filename, std::ios::app);
    m_currentSize = 0;
}

} // namespace loglib
