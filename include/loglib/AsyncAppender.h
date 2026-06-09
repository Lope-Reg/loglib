#pragma once
#include "LogAppender.h"
#include <queue>
#include <thread>
#include <condition_variable>
#include <atomic>

namespace loglib {

// 装饰器模式: 包装任意 Appender，使其异步
class AsyncAppender : public LogAppender {
public:
    using ptr = std::shared_ptr<AsyncAppender>;
    using LogTask = std::pair<LogLevel, std::string>;

    explicit AsyncAppender(LogAppender::ptr inner);
    ~AsyncAppender();

    void log(LogLevel level, const std::string& msg) override;

    void start();
    void stop();

private:
    void loop();

    LogAppender::ptr m_inner;
    std::queue<LogTask> m_queue;       // STL: queue
    std::mutex m_mutex;
    std::condition_variable m_cond;    // C++11: 条件变量
    std::thread m_thread;              // C++11: 线程
    std::atomic<bool> m_running{false}; // C++11: 原子操作
};

} // namespace loglib
