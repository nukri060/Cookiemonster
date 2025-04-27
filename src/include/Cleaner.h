#pragma once
#include <string>
#include <vector>
#include <iostream>
#include <windows.h>
#include <shlobj.h>
#include <fstream>
#include <chrono>
#include <iomanip>
#include <sstream>

// Logging levels
enum class LogLevel {
    DEBUG,
    INFO,
    WARNING,
    ERROR
};

class Logger {
private:
    std::ofstream logFile;
    bool consoleOutput;
    static Logger* instance;

    Logger() : consoleOutput(true) {
        auto now = std::chrono::system_clock::now();
        auto time = std::chrono::system_clock::to_time_t(now);
        std::stringstream ss;
        ss << "cookiemonster_" << std::put_time(std::localtime(&time), "%Y%m%d_%H%M%S") << ".log";
        logFile.open(ss.str(), std::ios::app);
    }

public:
    static Logger& getInstance() {
        if (!instance) {
            instance = new Logger();
        }
        return *instance;
    }

    void log(LogLevel level, const std::string& message) {
        std::string levelStr;
        switch (level) {
            case LogLevel::DEBUG: levelStr = "DEBUG"; break;
            case LogLevel::INFO: levelStr = "INFO"; break;
            case LogLevel::WARNING: levelStr = "WARNING"; break;
            case LogLevel::ERROR: levelStr = "ERROR"; break;
        }

        auto now = std::chrono::system_clock::now();
        auto time = std::chrono::system_clock::to_time_t(now);
        std::stringstream ss;
        ss << "[" << std::put_time(std::localtime(&time), "%Y-%m-%d %H:%M:%S") << "] "
           << "[" << levelStr << "] " << message;

        if (consoleOutput) {
            std::cout << ss.str() << std::endl;
        }
        logFile << ss.str() << std::endl;
        logFile.flush();
    }

    void setConsoleOutput(bool enable) {
        consoleOutput = enable;
    }

    ~Logger() {
        if (logFile.is_open()) {
            logFile.close();
        }
    }
};

struct TempFilesStats {
    int filesDeleted = 0;
    int errors = 0;
    uint64_t bytesFreed = 0;  // Size in bytes
    std::vector<std::string> errorMessages;
};

struct RecycleBinStats {
    int filesDeleted = 0;
    int errors = 0;
    uint64_t bytesFreed = 0;  // Size in bytes
    std::vector<std::string> errorMessages;
};

struct BrowserCacheStats {
    int filesDeleted = 0;
    int errors = 0;
    uint64_t bytesFreed = 0;  // Size in bytes
    std::string browserName;
    std::vector<std::string> errorMessages;
};

class Cleaner {
public:
    Cleaner();
    ~Cleaner();

    // Core cleaning functions
    bool cleanTempFiles(bool dryRun = false);
    bool cleanBrowserCache(bool dryRun = false);
    bool cleanRecycleBin(bool dryRun = false);
    bool cleanRegistry(bool dryRun = false);

    // Browser-specific cleaning functions
    bool cleanChromiumCache(bool dryRun = false);  // For Chrome and Edge
    bool cleanFirefoxCache(bool dryRun = false);

    // Utility functions
    bool isAdmin() const;
    void showStatistics() const;
    std::string formatSize(uint64_t bytes) const;  // Helper to format size
    std::vector<std::wstring> getTempDirectories() const;
    bool deleteFile(const std::string& path, bool dryRun = false);
    void setExcludedPaths(const std::vector<std::wstring>& paths);
    void setIncludedPaths(const std::vector<std::wstring>& paths);

private:
    // Helper methods
    bool deleteDirectory(const std::wstring& path, bool dryRun = false);
    std::wstring getLocalAppData() const;
    std::vector<std::wstring> getBrowserPaths() const;
    bool isPathExcluded(const std::wstring& path) const;
    bool isPathIncluded(const std::wstring& path) const;
    void logError(const std::string& operation, const std::string& error);
    
    TempFilesStats tempStats;
    RecycleBinStats recycleBinStats;
    std::vector<BrowserCacheStats> browserStats;
    std::vector<std::wstring> excludedPaths;
    std::vector<std::wstring> includedPaths;
}; 