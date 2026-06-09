#include "loglib/LoggerManager.h"
#include "loglib/PatternFormatter.h"
#include "loglib/LogAppender.h"

namespace loglib {

LoggerManager::LoggerManager() {
    m_root = std::make_shared<Logger>("root");
    m_root->addAppender(std::make_shared<StdoutAppender>());
    m_loggers["root"] = m_root;
}

// 内部方法: 不加锁，由调用方保证线程安全
Logger::ptr LoggerManager::createLoggerInternal(const std::string& name, LogLevel level) {
    auto it = m_loggers.find(name);
    if (it != m_loggers.end()) {
        return it->second;
    }

    // 工厂模式: 创建新日志器
    auto logger = std::make_shared<Logger>(name);
    logger->setLevel(level);
    logger->addAppender(std::make_shared<StdoutAppender>());
    m_loggers[name] = logger;
    return logger;
}

Logger::ptr LoggerManager::createLogger(const std::string& name, LogLevel level) {
    std::lock_guard<std::mutex> lock(m_mutex);
    return createLoggerInternal(name, level);
}

Logger::ptr LoggerManager::getLogger(const std::string& name) {
    std::lock_guard<std::mutex> lock(m_mutex);
    return createLoggerInternal(name, LogLevel::DEBUG);
}

void LoggerManager::init() {
    createLogger("system", LogLevel::INFO);
    createLogger("access", LogLevel::INFO);
}

} // namespace loglib
