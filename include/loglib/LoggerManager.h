#pragma once
#include <string>
#include <map>
#include <memory>
#include <mutex>
#include "Logger.h"

namespace loglib {

// 单例模式 + 工厂模式
class LoggerManager {
public:
    // 单例: Meyers' Singleton, C++11 保证线程安全
    static LoggerManager& getInstance() {
        static LoggerManager instance;
        return instance;
    }

    // 工厂方法
    Logger::ptr createLogger(const std::string& name,
                             LogLevel level = LogLevel::DEBUG);

    Logger::ptr getLogger(const std::string& name);
    Logger::ptr getRoot() { return m_root; }

    void init();

    // 禁用拷贝和赋值
    LoggerManager(const LoggerManager&) = delete;
    LoggerManager& operator=(const LoggerManager&) = delete;

private:
    LoggerManager();

    // 内部方法: 不加锁，避免递归锁死锁
    Logger::ptr createLoggerInternal(const std::string& name, LogLevel level);

    std::map<std::string, Logger::ptr> m_loggers;  // STL: map
    Logger::ptr m_root;
    std::mutex m_mutex;
};

} // namespace loglib
