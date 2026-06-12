#include "_internal.h"

namespace loglib {

Logger::ptr LoggerManager::Impl::createLoggerInternal(const std::string& name, LogLevel level) {
    auto it = loggers.find(name);
    if (it != loggers.end()) {
        return it->second;
    }
    auto logger = std::make_shared<Logger>(name);
    logger->setLevel(level);
    logger->addAppender(std::make_shared<StdoutAppender>());
    loggers[name] = logger;
    return logger;
}

LoggerManager::LoggerManager()
    : m_impl(std::make_unique<Impl>()) {
    m_impl->root = std::make_shared<Logger>("root");
    m_impl->root->addAppender(std::make_shared<StdoutAppender>());
    m_impl->loggers["root"] = m_impl->root;
}

LoggerManager::~LoggerManager() = default;

LoggerManager& LoggerManager::getInstance() {
    static LoggerManager instance;
    return instance;
}

Logger::ptr LoggerManager::createLogger(const std::string& name, LogLevel level) {
    std::lock_guard<std::mutex> lock(m_impl->mutex);
    return m_impl->createLoggerInternal(name, level);
}

Logger::ptr LoggerManager::getLogger(const std::string& name) {
    std::lock_guard<std::mutex> lock(m_impl->mutex);
    return m_impl->createLoggerInternal(name, LogLevel::DEBUG);
}

Logger::ptr LoggerManager::getRoot() {
    return m_impl->root;
}

void LoggerManager::init() {
    createLogger("system", LogLevel::INFO);
    createLogger("access", LogLevel::INFO);
}

} // namespace loglib
