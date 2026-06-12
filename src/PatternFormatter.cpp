#include "_internal.h"
#include <ctime>
#include <iomanip>

namespace loglib {

// LogFormatter 析构
LogFormatter::~LogFormatter() = default;

// 静态注册表
std::map<char, PatternFormatter::Impl::ItemCreator> PatternFormatter::Impl::creators = {
    {'d', [](LogEvent::ptr e) -> std::string {
        // 这里会被 init() 中带参数的版本覆盖，作为无参默认
        return "";
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

void PatternFormatter::Impl::init() {
    for (size_t i = 0; i < pattern.size(); ++i) {
        if (pattern[i] == '%' && i + 1 < pattern.size()) {
            char key = pattern[++i];
            if (key == 'T') {
                items.push_back({'T', "", [](LogEvent::ptr) { return "\t"; }});
            } else if (key == 'd' && i + 1 < pattern.size() && pattern[i + 1] == '{') {
                // 解析 %{...} 中的时间格式
                size_t end = pattern.find('}', i + 2);
                if (end != std::string::npos) {
                    std::string fmt = pattern.substr(i + 2, end - i - 2);
                    i = end;
                    items.push_back({'d', fmt, [fmt](LogEvent::ptr) -> std::string {
                        auto time = std::chrono::system_clock::to_time_t(
                            std::chrono::system_clock::now());
                        std::ostringstream oss;
                        oss << std::put_time(std::localtime(&time), fmt.c_str());
                        return oss.str();
                    }});
                    continue;
                }
            } else if (creators.count(key)) {
                items.push_back({key, "", creators[key]});
            }
        } else {
            std::string ch(1, pattern[i]);
            items.push_back({'\0', "", [ch](LogEvent::ptr) { return ch; }});
        }
    }
}

PatternFormatter::PatternFormatter(const std::string& pattern)
    : m_impl(std::make_unique<Impl>()) {
    m_impl->pattern = pattern;
    m_impl->init();
}

PatternFormatter::~PatternFormatter() = default;

std::string PatternFormatter::format(LogEvent::ptr event) {
    std::stringstream ss;
    for (auto& item : m_impl->items) {
        ss << item.handler(event);
    }
    return ss.str();
}

} // namespace loglib
