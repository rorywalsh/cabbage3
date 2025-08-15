#pragma once

#include <fstream>
#include <mutex>
#include <string>
#include <chrono>
#include <iomanip>
#include <sstream>

namespace cabbage {

class FileLogger {
public:
    enum class Level {
        DEBUG,
        INFO,
        WARNING,
        ERROR
    };

private:
    std::ofstream logFile;
    std::mutex logMutex;
    Level currentLevel;

    static FileLogger* instance;
    static std::mutex instanceMutex;

    FileLogger() : currentLevel(Level::DEBUG) {
        // Create log file in user's home directory for easy access
        std::string logPath = getLogPath();
        logFile.open(logPath, std::ios::out | std::ios::app);
        if (logFile.is_open()) {
            writeEntry(Level::INFO, "=== Cabbage Plugin Logger Started ===");
        }
    }

    std::string getLogPath() {
        const char* home = getenv("HOME");
        std::string homePath = home ? home : "/tmp";
        return homePath + "/cabbage_plugin_debug.log";
    }

    std::string getCurrentTimestamp() {
        auto now = std::chrono::system_clock::now();
        auto time_t = std::chrono::system_clock::to_time_t(now);
        auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
            now.time_since_epoch()) % 1000;
        
        std::stringstream ss;
        ss << std::put_time(std::localtime(&time_t), "%Y-%m-%d %H:%M:%S");
        ss << '.' << std::setfill('0') << std::setw(3) << ms.count();
        return ss.str();
    }

    std::string levelToString(Level level) {
        switch (level) {
            case Level::DEBUG: return "[DEBUG]";
            case Level::INFO:  return "[INFO ]";
            case Level::WARNING: return "[WARN ]";
            case Level::ERROR: return "[ERROR]";
            default: return "[UNKN ]";
        }
    }

    void writeEntry(Level level, const std::string& message) {
        if (level < currentLevel || !logFile.is_open()) {
            return;
        }

        std::lock_guard<std::mutex> lock(logMutex);
        logFile << getCurrentTimestamp() << " " << levelToString(level) << " " << message << std::endl;
        logFile.flush(); // Ensure immediate write
    }

public:
    ~FileLogger() {
        if (logFile.is_open()) {
            writeEntry(Level::INFO, "=== Cabbage Plugin Logger Stopped ===");
            logFile.close();
        }
    }

    static FileLogger& getInstance() {
        std::lock_guard<std::mutex> lock(instanceMutex);
        if (!instance) {
            instance = new FileLogger();
        }
        return *instance;
    }

    void setLevel(Level level) {
        currentLevel = level;
    }

    void debug(const std::string& message) {
        writeEntry(Level::DEBUG, message);
    }

    void info(const std::string& message) {
        writeEntry(Level::INFO, message);
    }

    void warning(const std::string& message) {
        writeEntry(Level::WARNING, message);
    }

    void error(const std::string& message) {
        writeEntry(Level::ERROR, message);
    }

    std::string getLogFilePath() {
        return getLogPath();
    }
};

// Convenience macros for easy logging
#define CABBAGE_LOG_DEBUG(msg) cabbage::FileLogger::getInstance().debug(msg)
#define CABBAGE_LOG_INFO(msg) cabbage::FileLogger::getInstance().info(msg)
#define CABBAGE_LOG_WARNING(msg) cabbage::FileLogger::getInstance().warning(msg)
#define CABBAGE_LOG_ERROR(msg) cabbage::FileLogger::getInstance().error(msg)

// Enhanced macros that include function and line information
#define CABBAGE_LOG_DEBUG_LOC(msg) do { \
    std::stringstream ss; \
    ss << "[" << __FUNCTION__ << ":" << __LINE__ << "] " << msg; \
    cabbage::FileLogger::getInstance().debug(ss.str()); \
} while(0)

#define CABBAGE_LOG_INFO_LOC(msg) do { \
    std::stringstream ss; \
    ss << "[" << __FUNCTION__ << ":" << __LINE__ << "] " << msg; \
    cabbage::FileLogger::getInstance().info(ss.str()); \
} while(0)

#define CABBAGE_LOG_WARNING_LOC(msg) do { \
    std::stringstream ss; \
    ss << "[" << __FUNCTION__ << ":" << __LINE__ << "] " << msg; \
    cabbage::FileLogger::getInstance().warning(ss.str()); \
} while(0)

#define CABBAGE_LOG_ERROR_LOC(msg) do { \
    std::stringstream ss; \
    ss << "[" << __FUNCTION__ << ":" << __LINE__ << "] " << msg; \
    cabbage::FileLogger::getInstance().error(ss.str()); \
} while(0)

} // namespace cabbage
