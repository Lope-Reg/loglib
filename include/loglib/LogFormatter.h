#pragma once
#include <string>
#include <memory>
#include "LogEvent.h"

namespace loglib {

// 抽象基类 —— 策略模式的 Strategy 接口
class LogFormatter {
public:
    using ptr = std::shared_ptr<LogFormatter>;

    virtual ~LogFormatter() = default;  // C++11: = default

    // 纯虚函数 —— 多态的基础
    virtual std::string format(LogEvent::ptr event) = 0;
};

} // namespace loglib
