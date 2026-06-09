#include "loglib/AsyncAppender.h"

namespace loglib {

AsyncAppender::AsyncAppender(LogAppender::ptr inner)
    : m_inner(std::move(inner)) {
    start();
}

AsyncAppender::~AsyncAppender() {
    stop();
}

void AsyncAppender::start() {
    m_running = true;
    // C++11: lambda + thread
    m_thread = std::thread([this]() { this->loop(); });
}

void AsyncAppender::stop() {
    m_running = false;
    m_cond.notify_all();
    if (m_thread.joinable()) {
        m_thread.join();
    }
}

void AsyncAppender::log(LogLevel level, const std::string& msg) {
    // 生产者: 加入队列
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_queue.push({level, msg});
    }
    m_cond.notify_one();
}

void AsyncAppender::loop() {
    // 消费者线程
    while (m_running) {
        std::unique_lock<std::mutex> lock(m_mutex);
        // C++11: 条件变量等待
        m_cond.wait(lock, [this]() {
            return !m_queue.empty() || !m_running;
        });

        while (!m_queue.empty()) {
            auto task = std::move(m_queue.front());
            m_queue.pop();
            lock.unlock();
            m_inner->log(task.first, task.second);
            lock.lock();
        }
    }

    // 排空剩余日志
    std::lock_guard<std::mutex> lock(m_mutex);
    while (!m_queue.empty()) {
        auto task = std::move(m_queue.front());
        m_queue.pop();
        m_inner->log(task.first, task.second);
    }
}

} // namespace loglib
