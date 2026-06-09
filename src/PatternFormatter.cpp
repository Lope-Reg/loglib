#include "loglib/PatternFormatter.h"
#include <ctime>
#include <iomanip>
#include <sstream>

namespace loglib {

// 静态成员初始化 —— 注册格式化项
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
    // 解析 pattern，将每个 %x 对应的 handler 存入 m_items
    for (size_t i = 0; i < m_pattern.size(); ++i) {
        if (m_pattern[i] == '%' && i + 1 < m_pattern.size()) {
            char key = m_pattern[++i];
            if (key == 'T') {
                // 分隔符 tab
                m_items.push_back({'T', "", [](LogEvent::ptr) { return "\t"; }});
            } else if (s_creators.count(key)) {
                // 带参数的时间格式
                if (key == 'd' && i + 1 < m_pattern.size() && m_pattern[i + 1] == '{') {
                    // 解析 %{...} 中的参数
                    size_t end = m_pattern.find('}', i + 2);
                    if (end != std::string::npos) {
                        std::string fmt = m_pattern.substr(i + 2, end - i - 2);
                        i = end;  // 跳过参数部分
                        // 为时间格式创建专门的 handler
                        m_items.push_back({'d', fmt, [fmt](LogEvent::ptr e) -> std::string {
                            auto time = std::chrono::system_clock::to_time_t(e->getTime());
                            std::ostringstream oss;
                            oss << std::put_time(std::localtime(&time), fmt.c_str());
                            return oss.str();
                        }});
                        continue;
                    }
                }
                m_items.push_back({key, "", s_creators[key]});
            }
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
