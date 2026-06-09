#pragma once
#include "LoggerManager.h"

// Facade 模式: 宏封装了全部细节

#define LOG_LEVEL(logger, level, msg) \
    (logger)->log(level, __FILE__, __LINE__, msg)

#define LOG_DEBUG(logger, msg) LOG_LEVEL(logger, loglib::LogLevel::DEBUG, msg)
#define LOG_INFO(logger, msg)  LOG_LEVEL(logger, loglib::LogLevel::INFO,  msg)
#define LOG_WARN(logger, msg)  LOG_LEVEL(logger, loglib::LogLevel::WARN,  msg)
#define LOG_ERROR(logger, msg) LOG_LEVEL(logger, loglib::LogLevel::ERROR, msg)
#define LOG_FATAL(logger, msg) LOG_LEVEL(logger, loglib::LogLevel::FATAL, msg)

// 流式日志宏
#define LOG_STREAM(logger, level) \
    (logger)->stream(level, __FILE__, __LINE__)

#define LOG_DEBUG_S(logger) LOG_STREAM(logger, loglib::LogLevel::DEBUG)
#define LOG_INFO_S(logger)  LOG_STREAM(logger, loglib::LogLevel::INFO)
#define LOG_WARN_S(logger)  LOG_STREAM(logger, loglib::LogLevel::WARN)
#define LOG_ERROR_S(logger) LOG_STREAM(logger, loglib::LogLevel::ERROR)
#define LOG_FATAL_S(logger) LOG_STREAM(logger, loglib::LogLevel::FATAL)

// 快捷宏: 使用 root 日志器
#define ROOT_LOG_DEBUG(msg) LOG_DEBUG(loglib::LoggerManager::getInstance().getRoot(), msg)
#define ROOT_LOG_INFO(msg)  LOG_INFO(loglib::LoggerManager::getInstance().getRoot(), msg)
#define ROOT_LOG_WARN(msg)  LOG_WARN(loglib::LoggerManager::getInstance().getRoot(), msg)
#define ROOT_LOG_ERROR(msg) LOG_ERROR(loglib::LoggerManager::getInstance().getRoot(), msg)
#define ROOT_LOG_FATAL(msg) LOG_FATAL(loglib::LoggerManager::getInstance().getRoot(), msg)
