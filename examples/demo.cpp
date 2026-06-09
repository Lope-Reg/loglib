#include "loglib/LogMacros.h"
#include "loglib/LoggerManager.h"
#include "loglib/LogAppender.h"
#include "loglib/AsyncAppender.h"
#include "loglib/PatternFormatter.h"
#include <thread>
#include <vector>

int main() {
    using namespace loglib;

    // 获取日志管理器 (单例)
    auto& mgr = LoggerManager::getInstance();
    mgr.init();

    // 获取 logger
    auto logger = mgr.getLogger("demo");

    // 设置自定义格式
    logger->setFormatter(std::make_shared<PatternFormatter>(
        "%d{%H:%M:%S} [%p] <%t> %m%n"));

    // 添加文件输出器
    logger->addAppender(std::make_shared<FileAppender>("demo.log"));

    // 添加异步滚动文件输出器
    auto rolling = std::make_shared<RollingFileAppender>("logs/app", 5 * 1024 * 1024);
    logger->addAppender(std::make_shared<AsyncAppender>(rolling));

    // ---- 基本日志 ----
    LOG_INFO(logger, "Application started");
    LOG_DEBUG(logger, "Debug message");
    LOG_WARN(logger, "Something might be wrong");
    LOG_ERROR(logger, "An error occurred");
    LOG_FATAL(logger, "Fatal error!");

    // ---- 流式日志 ----
    LOG_INFO_S(logger) << "User " << "admin" << " logged in, id=" << 42;
    LOG_DEBUG_S(logger) << "Value: " << 3.14 << ", flag: " << true;

    // ---- 多线程测试 ----
    std::vector<std::thread> threads;
    for (int i = 0; i < 4; ++i) {
        threads.emplace_back([logger, i]() {
            for (int j = 0; j < 10; ++j) {
                LOG_INFO_S(logger) << "Thread " << i << " msg " << j;
            }
        });
    }
    for (auto& t : threads) {
        t.join();
    }

    // ---- 快捷宏 ----
    ROOT_LOG_INFO("Using root logger directly");
    ROOT_LOG_WARN("This is a warning from root");

    return 0;
}
