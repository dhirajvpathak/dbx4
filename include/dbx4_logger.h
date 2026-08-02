#pragma once
#include <iostream>
#include <fstream>
#include <sstream>
#include <mutex>
#include <chrono>
#include <iomanip>
#include <memory>
namespace dbx4 {
enum class LogLevel {
    DEBUG = 0,
    INFO = 1,
    WARN = 2,
    ERROR = 3,
    CRITICAL = 4
};
class Logger {
private:
    static std::unique_ptr<Logger> instance_;
    std::ofstream log_file_;
    LogLevel min_level_;
    std::mutex lock_;
    Logger() : min_level_(LogLevel::INFO) {
        log_file_.open("dbx4.log", std::ios::app);
    }
public:
    static Logger& get() {
        if (!instance_) {
            instance_ = std::unique_ptr<Logger>(new Logger());
        }
        return *instance_;
    }
    void set_level(LogLevel level) {
        std::lock_guard<std::mutex> guard(lock_);
        min_level_ = level;
    }
    void log(LogLevel level, const std::string& source, const std::string& message) {
        if (level < min_level_) return;
        std::lock_guard<std::mutex> guard(lock_);
        auto now = std::chrono::system_clock::now();
        auto time = std::chrono::system_clock::to_time_t(now);
        auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()) % 1000;
        std::ostringstream timestamp;
        timestamp << std::put_time(std::localtime(&time), "%Y-%m-%d %H:%M:%S") << "." << std::setfill('0') << std::setw(3) << ms.count();
        std::string level_str;
        switch(level) {
            case LogLevel::DEBUG: level_str = "DEBUG"; break;
            case LogLevel::INFO: level_str = "INFO"; break;
            case LogLevel::WARN: level_str = "WARN"; break;
            case LogLevel::ERROR: level_str = "ERROR"; break;
            case LogLevel::CRITICAL: level_str = "CRITICAL"; break;
        }
        std::string log_line = "[" + timestamp.str() + "] [" + level_str + "] [" + source + "] " + message;
        if (log_file_.is_open()) {
            log_file_ << log_line << std::endl;
            log_file_.flush();
        }
    }
    ~Logger() {
        if (log_file_.is_open()) {
            log_file_.close();
        }
    }
};
std::unique_ptr<Logger> Logger::instance_ = nullptr;
#define LOG_DEBUG(source, msg) dbx4::Logger::get().log(dbx4::LogLevel::DEBUG, source, msg)
#define LOG_INFO(source, msg) dbx4::Logger::get().log(dbx4::LogLevel::INFO, source, msg)
#define LOG_WARN(source, msg) dbx4::Logger::get().log(dbx4::LogLevel::WARN, source, msg)
#define LOG_ERROR(source, msg) dbx4::Logger::get().log(dbx4::LogLevel::ERROR, source, msg)
#define LOG_CRITICAL(source, msg) dbx4::Logger::get().log(dbx4::LogLevel::CRITICAL, source, msg)
}
