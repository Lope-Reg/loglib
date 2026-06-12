// _internal.h —— 仅供 .cpp 文件使用，不对外暴露
#pragma once

#include "loglib/loglib.h"

#include <mutex>
#include <fstream>
#include <sstream>
#include <vector>
#include <map>
#include <queue>
#include <thread>
#include <condition_variable>
#include <atomic>
#include <functional>
#include <chrono>

namespace loglib {

// ======================== LogEvent::Impl ========================
struct LogEvent::Impl {
    const char* file;
    int line;
    LogLevel level;
    std::string loggerName;
    std::string content;
    uint64_t threadId;
    std::chrono::system_clock::time_point time;
};

// ======================== PatternFormatter::Impl ========================
struct PatternFormatter::Impl {
    std::string pattern;

    struct FormatItem {
        char key;
        std::string param;
        std::function<std::string(LogEvent::ptr)> handler;
    };

    std::vector<FormatItem> items;
    using ItemCreator = std::function<std::string(LogEvent::ptr)>;
    static std::map<char, ItemCreator> creators;

    void init();
};

// ======================== StdoutAppender::Impl ========================
struct StdoutAppender::Impl {
    std::mutex mutex;
};

// ======================== FileAppender::Impl ========================
struct FileAppender::Impl {
    std::string filename;
    std::ofstream ofstream;
    std::mutex mutex;
};

// ======================== RollingFileAppender::Impl ========================
struct RollingFileAppender::Impl {
    std::string basename;
    size_t maxSize;
    size_t currentSize = 0;
    int fileIndex = 0;
    std::ofstream ofstream;
    std::mutex mutex;

    void roll();
};

// ======================== AsyncAppender::Impl ========================
struct AsyncAppender::Impl {
    LogAppender::ptr inner;
    std::queue<std::pair<LogLevel, std::string>> queue;
    std::mutex mutex;
    std::condition_variable cond;
    std::thread thread;
    std::atomic<bool> running{false};

    void start();
    void stop();
    void loop();
};

// ======================== Logger::Impl ========================
struct Logger::Impl {
    std::string name;
    LogLevel level = LogLevel::DEBUG;
    LogFormatter::ptr formatter;
    std::vector<LogAppender::ptr> appenders;
    std::mutex mutex;
};

// ======================== Logger::LogStream::Impl ========================
struct Logger::LogStream::Impl {
    Logger::ptr logger;
    LogLevel level;
    const char* file;
    int line;
    std::stringstream ss;
};

// ======================== LoggerManager::Impl ========================
struct LoggerManager::Impl {
    std::map<std::string, Logger::ptr> loggers;
    Logger::ptr root;
    std::mutex mutex;

    Logger::ptr createLoggerInternal(const std::string& name, LogLevel level);
};

} // namespace loglib
