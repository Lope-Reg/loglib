#include "_internal.h"
#include <iostream>

namespace loglib {

// ---- LogAppender ----
LogAppender::~LogAppender() = default;
void LogAppender::setLevel(LogLevel level) { m_level = level; }
LogLevel LogAppender::getLevel() const { return m_level; }
LogAppender::LogAppender() : m_level(LogLevel::DEBUG) {}

// ---- StdoutAppender ----
StdoutAppender::StdoutAppender() : m_impl(std::make_unique<Impl>()) {}
StdoutAppender::~StdoutAppender() = default;

void StdoutAppender::log(LogLevel level, const std::string& msg) {
    if (level < m_level) return;
    std::lock_guard<std::mutex> lock(m_impl->mutex);

    const char* color = "";
    switch (level) {
        case LogLevel::DEBUG: color = "\033[36m"; break; // cyan
        case LogLevel::INFO:  color = "\033[32m"; break; // green
        case LogLevel::WARN:  color = "\033[33m"; break; // yellow
        case LogLevel::ERROR: color = "\033[31m"; break; // red
        case LogLevel::FATAL: color = "\033[35m"; break; // magenta
        default: break;
    }
    std::cout << color << msg << "\033[0m";
}

// ---- FileAppender ----
FileAppender::FileAppender(const std::string& filename)
    : m_impl(std::make_unique<Impl>()) {
    m_impl->filename = filename;
    m_impl->ofstream.open(filename, std::ios::app);
}

FileAppender::~FileAppender() {
    if (m_impl->ofstream.is_open()) {
        m_impl->ofstream.close();
    }
}

void FileAppender::log(LogLevel level, const std::string& msg) {
    if (level < m_level) return;
    std::lock_guard<std::mutex> lock(m_impl->mutex);
    m_impl->ofstream << msg;
    m_impl->ofstream.flush();
}

// ---- RollingFileAppender ----
RollingFileAppender::RollingFileAppender(const std::string& basename, size_t maxSize)
    : m_impl(std::make_unique<Impl>()) {
    m_impl->basename = basename;
    m_impl->maxSize = maxSize;
    m_impl->roll();
}

RollingFileAppender::~RollingFileAppender() = default;

void RollingFileAppender::Impl::roll() {
    if (ofstream.is_open()) {
        ofstream.close();
    }
    std::string filename = basename + "." + std::to_string(fileIndex++) + ".log";
    ofstream.open(filename, std::ios::app);
    currentSize = 0;
}

void RollingFileAppender::log(LogLevel level, const std::string& msg) {
    if (level < m_level) return;
    std::lock_guard<std::mutex> lock(m_impl->mutex);

    m_impl->currentSize += msg.size();
    if (m_impl->currentSize >= m_impl->maxSize) {
        m_impl->roll();
    }
    m_impl->ofstream << msg;
    m_impl->ofstream.flush();
}

// ---- AsyncAppender ----
void AsyncAppender::Impl::start() {
    running = true;
    thread = std::thread([this]() { loop(); });
}

void AsyncAppender::Impl::stop() {
    running = false;
    cond.notify_all();
    if (thread.joinable()) {
        thread.join();
    }
}

void AsyncAppender::Impl::loop() {
    while (running) {
        std::unique_lock<std::mutex> lock(mutex);
        cond.wait(lock, [this]() {
            return !queue.empty() || !running;
        });

        while (!queue.empty()) {
            auto task = std::move(queue.front());
            queue.pop();
            lock.unlock();
            inner->log(task.first, task.second);
            lock.lock();
        }
    }

    // 排空剩余
    std::lock_guard<std::mutex> lock(mutex);
    while (!queue.empty()) {
        auto task = std::move(queue.front());
        queue.pop();
        inner->log(task.first, task.second);
    }
}

AsyncAppender::AsyncAppender(LogAppender::ptr inner)
    : m_impl(std::make_unique<Impl>()) {
    m_impl->inner = std::move(inner);
    m_impl->start();
}

AsyncAppender::~AsyncAppender() {
    m_impl->stop();
}

void AsyncAppender::log(LogLevel level, const std::string& msg) {
    {
        std::lock_guard<std::mutex> lock(m_impl->mutex);
        m_impl->queue.push({level, msg});
    }
    m_impl->cond.notify_one();
}

} // namespace loglib
