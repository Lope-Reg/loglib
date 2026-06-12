#include "_internal.h"
#include <thread>

namespace loglib {

LogEvent::LogEvent(const char* file, int line, LogLevel level,
                   const std::string& loggerName, const std::string& content)
    : m_impl(std::make_unique<Impl>()) {
    m_impl->file = file;
    m_impl->line = line;
    m_impl->level = level;
    m_impl->loggerName = loggerName;
    m_impl->content = content;
    m_impl->time = std::chrono::system_clock::now();
    m_impl->threadId = std::hash<std::thread::id>{}(std::this_thread::get_id());
}

LogEvent::~LogEvent() = default;

const char* LogEvent::getFile() const { return m_impl->file; }
int LogEvent::getLine() const { return m_impl->line; }
LogLevel LogEvent::getLevel() const { return m_impl->level; }
const std::string& LogEvent::getLoggerName() const { return m_impl->loggerName; }
const std::string& LogEvent::getContent() const { return m_impl->content; }
uint64_t LogEvent::getThreadId() const { return m_impl->threadId; }

} // namespace loglib
