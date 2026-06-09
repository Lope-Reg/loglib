#include "loglib/LogMacros.h"
#include "loglib/LoggerManager.h"
#include "loglib/LogAppender.h"
#include "loglib/AsyncAppender.h"
#include "loglib/PatternFormatter.h"
#include <thread>
#include <vector>
#include <unistd.h>

int main(void){
    using namespace loglib;
    auto &loggerInstance=LoggerManager::getInstance();
    loggerInstance.init();

    auto logger=loggerInstance.getLogger("Miracast");


    logger->setFormatter(std::make_shared<PatternFormatter>(
        "%d{%H:%M:%S} [%p] <%t> %m%n"));


    logger->addAppender(std::make_shared<FileAppender>("MediaDecoder.log"));


    LOG_INFO(logger,"收到HTTP请求---HandleHttp.do.");
    LOG_ERROR(logger,"空指针.");
    LOG_WARN(logger,"非法参数.");

    auto rolling = std::make_shared<RollingFileAppender>("logs/logcat", 5 * 1024 * 1024);
    logger->addAppender(std::make_shared<AsyncAppender>(rolling));
    while(1){
        LOG_INFO(logger,"111111111111111111111");
        LOG_ERROR(logger,"222222222222222222222");
        LOG_WARN(logger,"333333333333333333333333");
        sleep(1);
    }
    
    return 0;
}