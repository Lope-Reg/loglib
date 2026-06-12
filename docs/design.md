# LogLib 设计与实现文档

## 目录

1. [项目概述](#1-项目概述)
2. [架构总览](#2-架构总览)
3. [C++ 特性覆盖矩阵](#3-c-特性覆盖矩阵)
4. [分步实现指南](#4-分步实现指南)
   - Step 1: 基础设施 —— 日志级别与事件
   - Step 2: 格式化器 —— 策略模式 + 继承 + 多态
   - Step 3: 输出器 —— 继承 + 多态 + STL
   - Step 4: 日志器 —— 封装 + 智能指针 + 组合
   - Step 5: 日志管理器 —— 单例模式 + 工厂模式
   - Step 6: 异步日志 —— C++11 线程 + 条件变量
   - Step 7: 线程安全 —— 互斥锁 + 原子操作
   - Step 8: 宏与门面 —— 设计模式 Facade
5. [类图总览](#5-类图总览)
6. [构建与测试](#6-构建与测试)

---

## 1. 项目概述

### 目标

构建一个支持以下功能的日志库：

| 功能 | 说明 |
|------|------|
| 多日志级别 | DEBUG / INFO / WARN / ERROR / FATAL |
| 多输出目标 | 控制台、文件、滚动文件、自定义 Sink |
| 格式化可配置 | 时间戳、线程ID、文件名、行号、自定义格式 |
| 线程安全 | 多线程并发写日志不乱序 |
| 异步写入 | 日志线程异步消费，不阻塞业务线程 |
| 单例管理 | 全局统一管理日志器实例 |

### C++ 特性覆盖

| 特性 | 在哪里用 |
|------|----------|
| **封装** | LogEvent、Logger、Formatter 私有成员 + public 接口 |
| **继承** | LogFormatter 抽象基类 → PatternFormatter；LogAppender 基类 → Console/File/Rolling |
| **多态** | 虚函数 `format()`、`log()`；基类指针调用派生类方法 |
| **STL** | `std::vector` 管理 Appender 列表，`std::map` 管理 Logger 集合，`std::stringstream` 拼接日志 |
| **C++11** | `enum class`、`using` 别名、`nullptr`、`override`/`final`、`lambda`、`auto`、范围 for、`std::chrono`、移动语义 |
| **智能指针** | `std::shared_ptr<Logger>` 管理日志器生命周期，`std::unique_ptr` 管理 Formatter/Appender |
| **设计模式** | Singleton（LoggerManager）、Strategy（Formatter）、Factory（createLogger）、Facade（宏） |

---

## 2. 架构总览

```
┌─────────────────────────────────────────────────────────────┐
│                        用户代码                              │
│   LOG_INFO(logger, "message")  ← Facade 宏                  │
└──────────────────────┬──────────────────────────────────────┘
                       │
                       ▼
┌──────────────────────────────────────────────────────────────┐
│                      Logger                                  │
│  - 持有 Formatter (策略)                                      │
│  - 持有 vector<shared_ptr<Appender>> (多输出)                 │
│  - log() 方法: 构造 LogEvent → 格式化 → 分发到所有 Appender    │
└────────────┬─────────────────────────────────────────────────┘
             │                          │
             ▼                          ▼
┌─────────────────────┐   ┌──────────────────────────────────┐
│    LogFormatter      │   │        LogAppender (抽象基类)     │
│  - format(LogEvent)  │   │  - log(level, formatted_string) │
│    (策略模式)         │   │                                  │
└─────────────────────┘   │  ┌──────────┐ ┌──────────────┐   │
                          │  │ Console   │ │ FileAppender │   │
                          │  │ Appender  │ │ RollingFile  │   │
                          │  └──────────┘ └──────────────┘   │
                          └──────────────────────────────────┘

┌──────────────────────────────────────────────────────────────┐
│                  LoggerManager (单例)                         │
│  - map<string, shared_ptr<Logger>>                           │
│  - getLogger(name) / createLogger(name, config)              │
└──────────────────────────────────────────────────────────────┘
```

---

## 3. C++ 特性覆盖矩阵

下面详细说明每个特性在代码中的具体位置：

### 3.1 封装 (Encapsulation)

```cpp
class LogEvent {
private:                              // 私有数据
    const char* m_file;
    int m_line;
    uint64_t m_threadId;
    std::string m_content;
    LogLevel m_level;
    std::chrono::system_clock::time_point m_time;
public:                               // 公开接口
    const char* getFile() const;
    int getLine() const;
    // ...
};
```

### 3.2 继承 (Inheritance)

```
LogAppender (抽象基类)
├── StdoutAppender      (控制台)
├── FileAppender        (文件)
└── RollingFileAppender (滚动文件)

LogFormatter (抽象基类)
└── PatternFormatter    (模式匹配格式化)
```

### 3.3 多态 (Polymorphism)

```cpp
// 基类指针，派生类对象
std::vector<std::shared_ptr<LogAppender>> m_appenders;

// 调用时自动分派到正确的派生类实现
for (auto& appender : m_appenders) {
    appender->log(level, msg);  // 虚函数调用
}
```

### 3.4 STL

```cpp
std::vector<shared_ptr<LogAppender>> m_appenders;       // 管理输出器
std::map<std::string, std::shared_ptr<Logger>> m_loggers; // 管理日志器
std::stringstream ss;                                      // 拼接日志内容
std::queue<LogEvent::ptr> m_queue;                         // 异步队列
```

### 3.5 C++11 新特性

```cpp
enum class LogLevel { DEBUG, INFO, WARN, ERROR, FATAL };  // 强类型枚举
using ptr = std::shared_ptr<Logger>;                       // using 别名
auto event = std::make_shared<LogEvent>(...);              // auto + make_shared
for (auto& appender : m_appenders) { ... }                 // 范围 for
std::thread t([this]() { this->loop(); });                 // lambda + thread
virtual ~LogAppender() = default;                          // = default
void log() override;                                       // override
```

### 3.6 智能指针

```cpp
std::shared_ptr<Logger>    m_logger;     // 日志器共享所有权
std::unique_ptr<Formatter> m_formatter;  // 格式化器独占所有权
std::make_shared<LogEvent>(...);         // 避免裸 new
```

### 3.7 设计模式

| 模式 | 应用 |
|------|------|
| **Singleton** | `LoggerManager::getInstance()` 全局唯一 |
| **Strategy** | `LogFormatter` 作为策略，可替换不同格式化方式 |
| **Factory** | `LoggerManager::createLogger()` 工厂方法创建日志器 |
| **Facade** | `LOG_INFO` 宏封装了构造事件、格式化、输出的全部细节 |

---

## 4. 分步实现指南

### Step 1: 基础设施 —— LogLevel + LogEvent

**涉及特性**: `enum class`、封装、`chrono`、`using`

创建文件 `include/loglib/LogLevel.h`:

```cpp
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

// C++11: constexpr 函数，编译期求值
const char* toString(LogLevel level);

} // namespace loglib
```

创建文件 `include/loglib/LogEvent.h`:

```cpp
#pragma once
#include <string>
#include <chrono>
#include <memory>
#include "LogLevel.h"

namespace loglib {

class LogEvent {
public:
    // C++11: using 别名
    using ptr = std::shared_ptr<LogEvent>;

    LogEvent(const char* file, int line, LogLevel level,
             const std::string& loggerName, const std::string& content)
        : m_file(file)
        , m_line(line)
        , m_level(level)
        , m_loggerName(loggerName)
        , m_content(content)
        , m_time(std::chrono::system_clock::now())  // C++11 chrono
        , m_threadId(std::hash<std::thread::id>{}(std::this_thread::get_id()))
    {}

    // 封装: public 接口访问 private 数据
    const char* getFile() const { return m_file; }
    int getLine() const { return m_line; }
    LogLevel getLevel() const { return m_level; }
    const std::string& getLoggerName() const { return m_loggerName; }
    const std::string& getContent() const { return m_content; }
    uint64_t getThreadId() const { return m_threadId; }
    std::chrono::system_clock::time_point getTime() const { return m_time; }

private:
    const char* m_file;       // 文件名
    int m_line;               // 行号
    LogLevel m_level;         // 日志级别
    std::string m_loggerName; // 日志器名称
    std::string m_content;    // 日志内容
    uint64_t m_threadId;      // 线程ID
    std::chrono::system_clock::time_point m_time;  // 时间戳
};

} // namespace loglib
```

创建 `src/LogLevel.cpp`:

```cpp
#include "loglib/LogLevel.h"

namespace loglib {

const char* toString(LogLevel level) {
    switch (level) {
        case LogLevel::DEBUG: return "DEBUG";
        case LogLevel::INFO:  return "INFO";
        case LogLevel::WARN:  return "WARN";
        case LogLevel::ERROR: return "ERROR";
        case LogLevel::FATAL: return "FATAL";
        default:              return "UNKNOWN";
    }
}

} // namespace loglib
```

**验证点**: 编译通过，`toString(LogLevel::INFO)` 返回 `"INFO"`。

---

### Step 2: 格式化器 —— 策略模式 + 继承 + 多态

**涉及特性**: 继承、纯虚函数、多态、策略模式、`stringstream`、`std::map`

创建 `include/loglib/LogFormatter.h`:

```cpp
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
```

创建 `include/loglib/PatternFormatter.h`:

```cpp
#pragma once
#include "LogFormatter.h"
#include <map>
#include <functional>
#include <vector>
#include <sstream>

namespace loglib {

// 继承: PatternFormatter 是 LogFormatter 的具体实现
class PatternFormatter : public LogFormatter {
public:
    // pattern 示例: "%d{%Y-%m-%d %H:%M:%S}%T%t%T[%p]%T%m%n"
    // %d = 时间, %t = 线程ID, %p = 级别, %m = 内容, %n = 换行
    explicit PatternFormatter(const std::string& pattern = "%d{%Y-%m-%d %H:%M:%S}%T[%p]%T%m%n");

    // override: 明确标识覆盖基类虚函数
    std::string format(LogEvent::ptr event) override;

private:
    // 格式化项: 一个格式化字符对应一个处理函数
    // C++11: std::function + lambda
    struct FormatItem {
        char key;
        std::string param;  // 例如时间格式
        std::function<std::string(LogEvent::ptr)> handler;
    };

    void init();  // 解析 pattern，构建 m_items

    std::string m_pattern;
    std::vector<FormatItem> m_items;  // STL: vector

    // C++11: using 别名
    using ItemCreator = std::function<std::string(LogEvent::ptr)>;
    // 注册表: 格式字符 → 创建函数
    static std::map<char, ItemCreator> s_creators;  // STL: map
};

} // namespace loglib
```

实现 `src/PatternFormatter.cpp`:

```cpp
#include "loglib/PatternFormatter.h"
#include <ctime>
#include <iomanip>
#include <sstream>

namespace loglib {

// 静态成员初始化
std::map<char, PatternFormatter::ItemCreator> PatternFormatter::s_creators = {
    // C++11: lambda 表达式
    {'d', [](LogEvent::ptr e) -> std::string {
        auto time = std::chrono::system_clock::to_time_t(e->getTime());
        std::ostringstream oss;
        oss << std::put_time(std::localtime(&time), "%Y-%m-%d %H:%M:%S");
        return oss.str();
    }},
    {'t', [](LogEvent::ptr e) -> std::string {
        return std::to_string(e->getThreadId());
    }},
    {'p', [](LogEvent::ptr e) -> std::string {
        return toString(e->getLevel());
    }},
    {'f', [](LogEvent::ptr e) -> std::string {
        return e->getFile();
    }},
    {'l', [](LogEvent::ptr e) -> std::string {
        return std::to_string(e->getLine());
    }},
    {'m', [](LogEvent::ptr e) -> std::string {
        return e->getContent();
    }},
    {'n', [](LogEvent::ptr) -> std::string {
        return "\n";
    }}
};

PatternFormatter::PatternFormatter(const std::string& pattern)
    : m_pattern(pattern) {
    init();
}

void PatternFormatter::init() {
    // 解析 pattern 字符串，将每个 %x 对应的 handler 存入 m_items
    // 简化实现: 遇到 %x 查 s_creators 注册表
    for (size_t i = 0; i < m_pattern.size(); ++i) {
        if (m_pattern[i] == '%' && i + 1 < m_pattern.size()) {
            char key = m_pattern[++i];
            if (s_creators.count(key)) {
                m_items.push_back({key, "", s_creators[key]});
            }
        } else if (m_pattern[i] == 'T') {
            // 分隔符 tab
            m_items.push_back({'T', "", [](LogEvent::ptr) { return "\t"; }});
        } else {
            // 普通字符
            std::string ch(1, m_pattern[i]);
            m_items.push_back({'\0', "", [ch](LogEvent::ptr) { return ch; }});
        }
    }
}

std::string PatternFormatter::format(LogEvent::ptr event) {
    std::stringstream ss;  // STL: stringstream
    for (auto& item : m_items) {  // C++11: 范围 for
        ss << item.handler(event);
    }
    return ss.str();
}

} // namespace loglib
```

**验证点**: 创建 `PatternFormatter`，传入 `LogEvent`，输出格式化字符串。

---

### Step 3: 输出器 —— 继承 + 多态 + STL + 智能指针

**涉及特性**: 继承、虚函数、多态、`std::vector`、`std::shared_ptr`、`std::mutex`

创建 `include/loglib/LogAppender.h`:

```cpp
#pragma once
#include <string>
#include <memory>
#include <mutex>
#include "LogLevel.h"

namespace loglib {

// 抽象基类 —— 多态接口
class LogAppender {
public:
    using ptr = std::shared_ptr<LogAppender>;

    virtual ~LogAppender() = default;  // C++11: = default

    // 纯虚函数: 子类必须实现
    virtual void log(LogLevel level, const std::string& msg) = 0;

    // 设置最低日志级别
    void setLevel(LogLevel level) { m_level = level; }
    LogLevel getLevel() const { return m_level; }

protected:
    LogLevel m_level = LogLevel::DEBUG;

    // C++11: mutable 用于在 const 方法中修改 mutex
    mutable std::mutex m_mutex;
};

// 具体输出器 1: 控制台
class StdoutAppender : public LogAppender {
public:
    void log(LogLevel level, const std::string& msg) override;

private:
    // 输出到 stdout，带颜色
    std::string colorize(LogLevel level, const std::string& msg);
};

// 具体输出器 2: 文件
class FileAppender : public LogAppender {
public:
    explicit FileAppender(const std::string& filename);
    ~FileAppender();

    void log(LogLevel level, const std::string& msg) override;

    // C++11: 禁用拷贝，允许移动
    FileAppender(const FileAppender&) = delete;
    FileAppender& operator=(const FileAppender&) = delete;
    FileAppender(FileAppender&&) noexcept = default;
    FileAppender& operator=(FileAppender&&) noexcept = default;

private:
    std::string m_filename;
    std::ofstream m_ofstream;
};

// 具体输出器 3: 滚动文件 (按大小切割)
class RollingFileAppender : public LogAppender {
public:
    RollingFileAppender(const std::string& basename,
                        size_t maxSize = 10 * 1024 * 1024);  // 默认 10MB

    void log(LogLevel level, const std::string& msg) override;

private:
    void roll();  // 滚动: 关闭旧文件，打开新文件

    std::string m_basename;
    size_t m_maxSize;
    size_t m_currentSize = 0;
    int m_fileIndex = 0;
    std::ofstream m_ofstream;
};

} // namespace loglib
```

实现 `src/LogAppender.cpp`:

```cpp
#include "loglib/LogAppender.h"
#include <iostream>
#include <fstream>
#include <iomanip>

namespace loglib {

// ---- StdoutAppender ----
std::string StdoutAppender::colorize(LogLevel level, const std::string& msg) {
    switch (level) {
        case LogLevel::DEBUG: return "\033[36m" + msg + "\033[0m"; // cyan
        case LogLevel::INFO:  return "\033[32m" + msg + "\033[0m"; // green
        case LogLevel::WARN:  return "\033[33m" + msg + "\033[0m"; // yellow
        case LogLevel::ERROR: return "\033[31m" + msg + "\033[0m"; // red
        case LogLevel::FATAL: return "\033[35m" + msg + "\033[0m"; // magenta
        default: return msg;
    }
}

void StdoutAppender::log(LogLevel level, const std::string& msg) {
    if (level < m_level) return;
    std::lock_guard<std::mutex> lock(m_mutex);  // C++11: RAII 锁
    std::cout << colorize(level, msg);
}

// ---- FileAppender ----
FileAppender::FileAppender(const std::string& filename)
    : m_filename(filename) {
    m_ofstream.open(filename, std::ios::app);  // 追加模式
}

FileAppender::~FileAppender() {
    if (m_ofstream.is_open()) {
        m_ofstream.close();
    }
}

void FileAppender::log(LogLevel level, const std::string& msg) {
    if (level < m_level) return;
    std::lock_guard<std::mutex> lock(m_mutex);
    m_ofstream << msg;
    m_ofstream.flush();
}

// ---- RollingFileAppender ----
RollingFileAppender::RollingFileAppender(const std::string& basename, size_t maxSize)
    : m_basename(basename), m_maxSize(maxSize) {
    roll();
}

void RollingFileAppender::log(LogLevel level, const std::string& msg) {
    if (level < m_level) return;
    std::lock_guard<std::mutex> lock(m_mutex);

    m_currentSize += msg.size();
    if (m_currentSize >= m_maxSize) {
        roll();
    }

    m_ofstream << msg;
    m_ofstream.flush();
}

void RollingFileAppender::roll() {
    if (m_ofstream.is_open()) {
        m_ofstream.close();
    }
    // 生成文件名: basename.0.log, basename.1.log, ...
    std::string filename = m_basename + "." + std::to_string(m_fileIndex++) + ".log";
    m_ofstream.open(filename, std::ios::app);
    m_currentSize = 0;
}

} // namespace loglib
```

**验证点**: `StdoutAppender` 打印带颜色日志，`FileAppender` 写入文件。

---

### Step 4: 日志器 —— 封装 + 智能指针 + 组合

**涉及特性**: 封装、`std::shared_ptr`、`std::vector`、组合模式、流式 API

创建 `include/loglib/Logger.h`:

```cpp
#pragma once
#include <string>
#include <vector>
#include <memory>
#include <sstream>
#include "LogLevel.h"
#include "LogEvent.h"
#include "LogFormatter.h"
#include "LogAppender.h"

namespace loglib {

class Logger : public std::enable_shared_from_this<Logger> {
public:
    using ptr = std::shared_ptr<Logger>;

    // 封装: 私有构造，只能通过 LoggerManager 创建
    explicit Logger(const std::string& name = "root");

    // 核心接口
    void log(LogLevel level, const std::string& file, int line, const std::string& msg);

    // 便捷方法
    void debug(const std::string& file, int line, const std::string& msg);
    void info(const std::string& file, int line, const std::string& msg);
    void warn(const std::string& file, int line, const std::string& msg);
    void error(const std::string& file, int line, const std::string& msg);
    void fatal(const std::string& file, int line, const std::string& msg);

    // 流式日志 —— C++11: 返回代理对象
    class LogStream {
    public:
        LogStream(Logger::ptr logger, LogLevel level, const char* file, int line);
        ~LogStream();  // 析构时提交日志

        // C++11: 模板 + 完美转发
        template<typename T>
        LogStream& operator<<(const T& value) {
            m_ss << value;
            return *this;
        }

    private:
        Logger::ptr m_logger;
        LogLevel m_level;
        const char* m_file;
        int m_line;
        std::stringstream m_ss;
    };

    LogStream stream(LogLevel level, const char* file, int line);

    // 配置接口
    void addAppender(LogAppender::ptr appender);
    void removeAppender(LogAppender::ptr appender);
    void setFormatter(LogFormatter::ptr formatter);
    void setLevel(LogLevel level) { m_level = level; }
    LogLevel getLevel() const { return m_level; }
    const std::string& getName() const { return m_name; }

private:
    std::string m_name;
    LogLevel m_level = LogLevel::DEBUG;
    LogFormatter::ptr m_formatter;                       // 智能指针: 共享所有权
    std::vector<LogAppender::ptr> m_appenders;           // STL + 智能指针
    mutable std::mutex m_mutex;
};

} // namespace loglib
```

实现 `src/Logger.cpp`:

```cpp
#include "loglib/Logger.h"
#include "loglib/PatternFormatter.h"

namespace loglib {

Logger::Logger(const std::string& name)
    : m_name(name)
    , m_formatter(std::make_shared<PatternFormatter>()) {  // 默认格式化器
}

void Logger::log(LogLevel level, const std::string& file, int line, const std::string& msg) {
    if (level < m_level) return;

    // 构造日志事件
    auto event = std::make_shared<LogEvent>(
        file.c_str(), line, level, m_name, msg);

    // 格式化
    std::string formatted = m_formatter->format(event);

    // 分发到所有 Appender —— 多态调用
    std::lock_guard<std::mutex> lock(m_mutex);
    for (auto& appender : m_appenders) {  // C++11: 范围 for
        appender->log(level, formatted);
    }
}

void Logger::debug(const std::string& file, int line, const std::string& msg) {
    log(LogLevel::DEBUG, file, line, msg);
}
void Logger::info(const std::string& file, int line, const std::string& msg) {
    log(LogLevel::INFO, file, line, msg);
}
void Logger::warn(const std::string& file, int line, const std::string& msg) {
    log(LogLevel::WARN, file, line, msg);
}
void Logger::error(const std::string& file, int line, const std::string& msg) {
    log(LogLevel::ERROR, file, line, msg);
}
void Logger::fatal(const std::string& file, int line, const std::string& msg) {
    log(LogLevel::FATAL, file, line, msg);
}

// ---- LogStream 实现 ----
Logger::LogStream::LogStream(Logger::ptr logger, LogLevel level,
                              const char* file, int line)
    : m_logger(std::move(logger))
    , m_level(level)
    , m_file(file)
    , m_line(line) {}

Logger::LogStream::~LogStream() {
    // RAII: 析构时自动提交日志
    m_logger->log(m_level, m_file, m_line, m_ss.str());
}

Logger::LogStream Logger::stream(LogLevel level, const char* file, int line) {
    return LogStream(shared_from_this(), level, file, line);
}

void Logger::addAppender(LogAppender::ptr appender) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_appenders.push_back(std::move(appender));  // C++11: 移动语义
}

void Logger::removeAppender(LogAppender::ptr appender) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_appenders.erase(
        std::remove(m_appenders.begin(), m_appenders.end(), appender),
        m_appenders.end());  // STL: erase-remove 惯用法
}

void Logger::setFormatter(LogFormatter::ptr formatter) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_formatter = std::move(formatter);
}

} // namespace loglib
```

**验证点**: 创建 Logger，添加 StdoutAppender，调用 `logger->info(...)` 输出到控制台。

---

### Step 5: 日志管理器 —— 单例模式 + 工厂模式

**涉及特性**: 单例模式、工厂模式、`std::map`、`std::shared_ptr`、`std::mutex`

创建 `include/loglib/LoggerManager.h`:

```cpp
#pragma once
#include <string>
#include <map>
#include <memory>
#include <mutex>
#include "Logger.h"

namespace loglib {

class LoggerManager {
public:
    // 单例模式: 线程安全的 Meyers' Singleton (C++11 保证)
    static LoggerManager& getInstance() {
        static LoggerManager instance;  // C++11: 线程安全的局部静态
        return instance;
    }

    // 工厂方法: 创建并注册日志器
    Logger::ptr createLogger(const std::string& name,
                             LogLevel level = LogLevel::DEBUG);

    // 获取已有日志器
    Logger::ptr getLogger(const std::string& name);

    // 获取 root 日志器
    Logger::ptr getRoot() { return m_root; }

    // 初始化默认配置
    void init();

    // 禁用拷贝和赋值 —— 单例
    LoggerManager(const LoggerManager&) = delete;
    LoggerManager& operator=(const LoggerManager&) = delete;

private:
    LoggerManager();  // 私有构造

    // 内部方法: 不加锁，避免递归锁死锁
    Logger::ptr createLoggerInternal(const std::string& name, LogLevel level);

    std::map<std::string, Logger::ptr> m_loggers;  // STL: map
    Logger::ptr m_root;
    std::mutex m_mutex;
};

} // namespace loglib
```

实现 `src/LoggerManager.cpp`:

```cpp
#include "loglib/LoggerManager.h"
#include "loglib/PatternFormatter.h"
#include "loglib/LogAppender.h"

namespace loglib {

LoggerManager::LoggerManager() {
    m_root = std::make_shared<Logger>("root");
    m_root->addAppender(std::make_shared<StdoutAppender>());
    m_loggers["root"] = m_root;
}

// ⚠️ 踩坑: 如果 getLogger() 持有锁再调用 createLogger()，
// 而 createLogger() 也加锁 → 死锁！
// 解决: 提取不加锁的内部方法，由调用方保证线程安全。
Logger::ptr LoggerManager::createLoggerInternal(const std::string& name, LogLevel level) {
    auto it = m_loggers.find(name);
    if (it != m_loggers.end()) {
        return it->second;
    }
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
    // 可以从配置文件读取，这里用默认配置
    createLogger("system", LogLevel::INFO);
    createLogger("access", LogLevel::INFO);
}

} // namespace loglib
```

**验证点**: `LoggerManager::getInstance().getLogger("test")` 返回同一个实例。

---

### Step 6: 异步日志 —— C++11 线程 + 条件变量

**涉及特性**: `std::thread`、`std::condition_variable`、`std::queue`、`std::atomic`、lambda

创建 `include/loglib/AsyncAppender.h`:

```cpp
#pragma once
#include "LogAppender.h"
#include <queue>
#include <thread>
#include <condition_variable>
#include <atomic>
#include <functional>

namespace loglib {

// 装饰器模式: 包装任意 Appender，使其异步
class AsyncAppender : public LogAppender {
public:
    using ptr = std::shared_ptr<AsyncAppender>;
    using LogTask = std::pair<LogLevel, std::string>;

    explicit AsyncAppender(LogAppender::ptr inner);
    ~AsyncAppender();

    void log(LogLevel level, const std::string& msg) override;

    // 启停
    void start();
    void stop();

private:
    void loop();  // 消费者线程主循环

    LogAppender::ptr m_inner;  // 被装饰的实际输出器
    std::queue<LogTask> m_queue;  // STL: queue
    std::mutex m_mutex;
    std::condition_variable m_cond;  // C++11: 条件变量
    std::thread m_thread;            // C++11: 线程
    std::atomic<bool> m_running{false};  // C++11: 原子操作
};

} // namespace loglib
```

实现 `src/AsyncAppender.cpp`:

```cpp
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
    m_cond.notify_all();  // 唤醒消费者
    if (m_thread.joinable()) {
        m_thread.join();  // 等待线程结束
    }
}

void AsyncAppender::log(LogLevel level, const std::string& msg) {
    // 生产者: 加入队列
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_queue.push({level, msg});
    }
    m_cond.notify_one();  // 唤醒消费者
}

void AsyncAppender::loop() {
    // 消费者线程
    while (m_running) {
        std::unique_lock<std::mutex> lock(m_mutex);
        // C++11: 条件变量等待，避免忙等
        m_cond.wait(lock, [this]() {
            return !m_queue.empty() || !m_running;
        });

        while (!m_queue.empty()) {
            auto task = std::move(m_queue.front());
            m_queue.pop();
            lock.unlock();  // 释放锁再写入
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
```

**验证点**: 用 `AsyncAppender` 包装 `FileAppender`，写入日志不阻塞主线程。

---

### Step 7: 线程安全总结

已通过以下机制保证线程安全：

| 机制 | 位置 | 说明 |
|------|------|------|
| `std::mutex` + `lock_guard` | Logger, Appender | 保护共享数据 |
| `std::atomic<bool>` | AsyncAppender | 原子标志位 |
| `std::condition_variable` | AsyncAppender | 高效等待/唤醒 |
| Meyers' Singleton | LoggerManager | C++11 保证线程安全初始化 |

---

### Step 8: 宏与门面 —— Facade 模式

**涉及特性**: 宏、预处理器、Facade 模式

创建 `include/loglib/LogMacros.h`:

```cpp
#pragma once
#include "LoggerManager.h"

// Facade 模式: 宏封装了获取 Logger、构造日志的全部细节
// 用户只需: LOG_INFO(logger, "message")

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
```

**验证点**: `LOG_INFO(logger, "hello")` 和 `LOG_INFO_S(logger) << "hello " << 42` 均可使用。

---

## 5. 类图总览

```
┌──────────────────────┐
│   LoggerManager      │  Singleton
│ - m_loggers: map<>   │  Factory
│ - m_root: Logger::ptr│
│ + getInstance()      │
│ + createLogger()     │
│ + getLogger()        │
└──────────┬───────────┘
           │ 持有
           ▼
┌──────────────────────┐       ┌─────────────────────┐
│      Logger          │       │    LogFormatter      │  Strategy
│ - m_name: string     │──────▶│ + format(event) = 0  │  (抽象)
│ - m_level: LogLevel  │       └──────────┬──────────┘
│ - m_formatter: ptr   │                  │ 继承
│ - m_appenders: vec<> │       ┌──────────▼──────────┐
│ + log()              │       │  PatternFormatter    │
│ + debug/info/...     │       │ - m_pattern: string  │
│ + stream()           │       │ - m_items: vector<>  │
└──────────┬───────────┘       │ + format() override  │
           │ 持有 (1:N)        └─────────────────────┘
           ▼
┌──────────────────────┐
│   LogAppender        │  (抽象)
│ + log(lvl, msg) = 0  │
│ - m_level: LogLevel  │
└──────────┬───────────┘
           │ 继承
    ┌──────┼──────────────┐
    ▼      ▼              ▼
┌────────┐ ┌──────────┐ ┌──────────────────┐
│Stdout  │ │File      │ │RollingFile       │
│Appender│ │Appender  │ │Appender          │
└────────┘ └──────────┘ └──────────────────┘

┌──────────────────────┐
│   AsyncAppender      │  Decorator
│ - m_inner: ptr       │──────▶ 包装任意 Appender
│ - m_queue: queue<>   │
│ - m_thread: thread   │
│ - m_cond: condvar    │
└──────────────────────┘

┌──────────────────────┐
│     LogEvent         │  Value Object
│ - m_file, m_line     │
│ - m_level            │
│ - m_threadId         │
│ - m_time             │
│ - m_content          │
└──────────────────────┘
```

---

## 6. 构建与测试

创建 `CMakeLists.txt`:

```cmake
cmake_minimum_required(VERSION 3.14)
project(loglib VERSION 1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 库源文件
set(SOURCES
    src/LogLevel.cpp
    src/PatternFormatter.cpp
    src/LogAppender.cpp
    src/Logger.cpp
    src/LoggerManager.cpp
    src/AsyncAppender.cpp
)

add_library(loglib STATIC ${SOURCES})
target_include_directories(loglib PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)

# 示例
add_executable(loglib_demo examples/demo.cpp)
target_link_libraries(loglib_demo loglib pthread)
```

创建 `examples/demo.cpp`:

```cpp
// 只需包含一个头文件
#include "loglib/loglib.h"
#include <thread>

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

    // ---- 流式日志 ----
    LOG_INFO_S(logger) << "User " << "admin" << " logged in, id=" << 42;

    // ---- 多线程测试 ----
    std::vector<std::thread> threads;
    for (int i = 0; i < 4; ++i) {
        threads.emplace_back([logger, i]() {
            for (int j = 0; j < 100; ++j) {
                LOG_INFO_S(logger) << "Thread " << i << " msg " << j;
            }
        });
    }
    for (auto& t : threads) {
        t.join();
    }

    // ---- 快捷宏 ----
    ROOT_LOG_INFO("Using root logger directly");

    return 0;
}
```

编译运行:

```bash
mkdir build && cd build
cmake .. && make -j$(nproc)
./loglib_demo
```

---

## 附录: C++ 特性速查表

| 特性 | 关键字/语法 | 在本项目中的用途 |
|------|-------------|-----------------|
| 封装 | `private/public` | LogEvent、Logger 数据隐藏 |
| 继承 | `class Derived : public Base` | Appender/Formatter 继承体系 |
| 多态 | `virtual ... = 0` / `override` | `log()`、`format()` 虚函数调用 |
| STL | `vector`, `map`, `queue`, `stringstream` | 容器管理 |
| enum class | `enum class LogLevel` | 强类型枚举 |
| using | `using ptr = shared_ptr<X>` | 类型别名 |
| auto | `auto x = ...` | 类型推导 |
| 范围 for | `for (auto& x : vec)` | 遍历容器 |
| lambda | `[](args){ body }` | 格式化项、线程函数 |
| 移动语义 | `std::move(x)` | 避免拷贝 |
| = default/delete | `~X() = default` / `X(const&) = delete` | 控制特殊成员函数 |
| override/final | `void f() override` | 明确覆盖意图 |
| 智能指针 | `shared_ptr`, `unique_ptr`, `make_shared` | 自动内存管理 |
| 线程 | `std::thread`, `mutex`, `condvar`, `atomic` | 异步日志、线程安全 |
| chrono | `system_clock::now()` | 高精度时间戳 |
| constexpr | `constexpr int N = 10` | 编译期常量 |
