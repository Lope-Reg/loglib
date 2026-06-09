#pragma once
#include "LogFormatter.h"
#include <map>
#include <functional>
#include <vector>
#include <sstream>

namespace loglib {

// 继承: PatternFormatter 是 LogFormatter 的具体实现
// 支持格式: %d{fmt} %t %p %f %l %m %n
class PatternFormatter : public LogFormatter {
public:
    explicit PatternFormatter(const std::string& pattern = "%d{%Y-%m-%d %H:%M:%S}%T[%p]%T%m%n");

    // override: 明确标识覆盖基类虚函数
    std::string format(LogEvent::ptr event) override;

private:
    struct FormatItem {
        char key;
        std::string param;
        std::function<std::string(LogEvent::ptr)> handler;
    };

    void init();

    std::string m_pattern;
    std::vector<FormatItem> m_items;  // STL: vector

    using ItemCreator = std::function<std::string(LogEvent::ptr)>;
    static std::map<char, ItemCreator> s_creators;  // STL: map
};

} // namespace loglib
