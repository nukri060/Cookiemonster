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

struct RegistryStats {
    int keysDeleted = 0;
    int valuesDeleted = 0;
    int errors = 0;
    std::vector<std::string> errorMessages;
};

// Structure for storing backup information
struct BackupInfo {
    std::string timestamp;
    std::string operationType;  // "temp", "registry", "browser", "recycle"
    std::string backupPath;
    uint64_t totalSize;
    std::vector<std::string> files;
    std::vector<std::pair<std::string, std::string>> registryKeys;  // Path and value
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

    // Registry cleaning functions
    bool cleanRegistryKey(HKEY hKey, const std::wstring& subKey, bool dryRun = false);
    std::vector<std::wstring> getObsoleteRegistryKeys() const;

    // Backup and restore functions
    bool createBackup(const std::string& operationType);
    bool restoreFromBackup(const std::string& backupPath);
    std::vector<BackupInfo> getAvailableBackups() const;
    bool deleteBackup(const std::string& backupPath);
    
    // Backup-specific cleaning functions
    bool cleanTempFilesWithBackup(bool dryRun = false);
    bool cleanRegistryWithBackup(bool dryRun = false);
    bool cleanBrowserCacheWithBackup(bool dryRun = false);
    bool cleanRecycleBinWithBackup(bool dryRun = false);

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
    RegistryStats registryStats;
    
    // Registry helper methods
    bool deleteRegistryKey(HKEY hKey, const std::wstring& subKey, bool dryRun = false);
    bool deleteRegistryValue(HKEY hKey, const std::wstring& subKey, const std::wstring& valueName, bool dryRun = false);

    // Backup helper methods
    std::string generateBackupPath(const std::string& operationType) const;
    bool backupFile(const std::string& sourcePath, const std::string& backupPath);
    bool backupRegistryKey(HKEY hKey, const std::wstring& subKey, const std::string& backupPath);
    bool restoreFile(const std::string& backupPath, const std::string& targetPath);
    bool restoreRegistryKey(const std::string& backupPath, HKEY hKey, const std::wstring& subKey);
    
    std::vector<BackupInfo> backupHistory;
}; 