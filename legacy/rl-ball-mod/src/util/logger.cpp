#include "logger.h"
#include <cstdarg>
#include <ctime>
#include <filesystem>

Logger& Logger::Get() {
    static Logger instance;
    return instance;
}

void Logger::Init(const std::string& filepath) {
    std::lock_guard<std::mutex> lock(m_mutex);
    if (m_initialized) return;

    std::filesystem::create_directories(
        std::filesystem::path(filepath).parent_path());

    m_file.open(filepath, std::ios::out | std::ios::trunc);
    m_initialized = m_file.is_open();

    if (m_initialized) {
        auto now = std::time(nullptr);
        char timebuf[64];
        std::strftime(timebuf, sizeof(timebuf), "%Y-%m-%d %H:%M:%S",
                      std::localtime(&now));
        m_file << "=== RL Ball Mod Log - " << timebuf << " ===" << std::endl;
    }
}

void Logger::SetLevel(LogLevel level) {
    m_level = level;
}

void Logger::Log(LogLevel level, const char* fmt, ...) {
    if (level < m_level || !m_initialized) return;

    std::lock_guard<std::mutex> lock(m_mutex);

    const char* prefix = "";
    switch (level) {
        case LogLevel::DEBUG: prefix = "[DEBUG] "; break;
        case LogLevel::INFO:  prefix = "[INFO]  "; break;
        case LogLevel::WARN:  prefix = "[WARN]  "; break;
        case LogLevel::ERR:   prefix = "[ERROR] "; break;
    }

    auto now = std::time(nullptr);
    char timebuf[32];
    std::strftime(timebuf, sizeof(timebuf), "%H:%M:%S", std::localtime(&now));

    char msgbuf[2048];
    va_list args;
    va_start(args, fmt);
    vsnprintf(msgbuf, sizeof(msgbuf), fmt, args);
    va_end(args);

    m_file << timebuf << " " << prefix << msgbuf << "\n";
    // Flush only on errors, not every line (major perf improvement)
    if (level >= LogLevel::ERR) m_file.flush();
}

void Logger::Close() {
    std::lock_guard<std::mutex> lock(m_mutex);
    if (m_file.is_open()) m_file.close();
    m_initialized = false;
}
