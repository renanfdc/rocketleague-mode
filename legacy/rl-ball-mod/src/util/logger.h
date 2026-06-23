#pragma once
#include <string>
#include <fstream>
#include <mutex>

enum class LogLevel { DEBUG, INFO, WARN, ERR };

class Logger {
public:
    static Logger& Get();
    void Init(const std::string& filepath);
    void SetLevel(LogLevel level);
    void Log(LogLevel level, const char* fmt, ...);
    void Close();

private:
    Logger() = default;
    std::ofstream m_file;
    std::mutex m_mutex;
    LogLevel m_level = LogLevel::INFO;
    bool m_initialized = false;
};

#define LOG_DEBUG(fmt, ...) Logger::Get().Log(LogLevel::DEBUG, fmt, ##__VA_ARGS__)
#define LOG_INFO(fmt, ...)  Logger::Get().Log(LogLevel::INFO,  fmt, ##__VA_ARGS__)
#define LOG_WARN(fmt, ...)  Logger::Get().Log(LogLevel::WARN,  fmt, ##__VA_ARGS__)
#define LOG_ERR(fmt, ...)   Logger::Get().Log(LogLevel::ERR,   fmt, ##__VA_ARGS__)
