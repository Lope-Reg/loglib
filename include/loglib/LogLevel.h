#pragma once

namespace loglib {

// C++11: enum class 强类型枚举，避免隐式转换
enum class LogLevel {
    DEBUG = 0,
    INFO  = 1,
    WARN  = 2,
    ERROR = 3,
    FATAL = 4
};

// 将 LogLevel 转为字符串
const char* toString(LogLevel level);

} // namespace loglib
