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
#include <memory>
#include <mutex>

/**
 * @brief Logging levels for the application
 */
enum class LogLevel {
    DEBUG,   ///< Debug information
    INFO,    ///< General information
    WARNING, ///< Warning messages
    ERROR    ///< Error messages
};

/**
 * @brief Singleton logger class for the application
 */
class Logger {
private:
    std::ofstream logFile;
    bool consoleOutput;
    static std::unique_ptr<Logger> instance;
    static std::mutex mutex;

    Logger() : consoleOutput(true) {
        auto now = std::chrono::system_clock::now();
        auto time = std::chrono::system_clock::to_time_t(now);
        std::stringstream ss;
        ss << "cookiemonster_" << std::put_time(std::localtime(&time), "%Y%m%d_%H%M%S") << ".log";
        logFile.open(ss.str(), std::ios::app);
    }

public:
    /**
     * @brief Get the singleton instance of the logger
     * @return Reference to the logger instance
     */
    static Logger& getInstance() {
        std::lock_guard<std::mutex> lock(mutex);
        if (!instance) {
            instance = std::make_unique<Logger>();
        }
        return *instance;
    }

    /**
     * @brief Log a message with specified level
     * @param level Logging level
     * @param message Message to log
     */
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

    /**
     * @brief Enable or disable console output
     * @param enable True to enable console output, false to disable
     */
    void setConsoleOutput(bool enable) {
        consoleOutput = enable;
    }

    ~Logger() {
        if (logFile.is_open()) {
            logFile.close();
        }
    }
};

/**
 * @brief Statistics for temporary files cleaning
 */
struct TempFilesStats {
    int filesDeleted = 0;           ///< Number of files deleted
    int errors = 0;                 ///< Number of errors encountered
    uint64_t bytesFreed = 0;        ///< Total size of freed space in bytes
    std::vector<std::string> errorMessages;  ///< List of error messages
};

/**
 * @brief Statistics for recycle bin cleaning
 */
struct RecycleBinStats {
    int filesDeleted = 0;           ///< Number of files deleted
    int errors = 0;                 ///< Number of errors encountered
    uint64_t bytesFreed = 0;        ///< Total size of freed space in bytes
    std::vector<std::string> errorMessages;  ///< List of error messages
};

/**
 * @brief Statistics for browser cache cleaning
 */
struct BrowserCacheStats {
    int filesDeleted = 0;           ///< Number of files deleted
    int errors = 0;                 ///< Number of errors encountered
    uint64_t bytesFreed = 0;        ///< Total size of freed space in bytes
    std::string browserName;        ///< Name of the browser
    std::vector<std::string> errorMessages;  ///< List of error messages
};

/**
 * @brief Statistics for registry cleaning
 */
struct RegistryStats {
    int keysDeleted = 0;            ///< Number of registry keys deleted
    int valuesDeleted = 0;          ///< Number of registry values deleted
    int errors = 0;                 ///< Number of errors encountered
    std::vector<std::string> errorMessages;  ///< List of error messages
};

/**
 * @brief Information about a backup operation
 */
struct BackupInfo {
    std::string timestamp;          ///< Backup creation timestamp
    std::string operationType;      ///< Type of operation ("temp", "registry", "browser", "recycle")
    std::string backupPath;         ///< Path to backup files
    uint64_t totalSize;             ///< Total size of backup in bytes
    std::vector<std::string> files; ///< List of backed up files
    std::vector<std::pair<std::string, std::string>> registryKeys;  ///< List of backed up registry keys
};

/**
 * @brief Main class for system cleaning operations
 */
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
    bool cleanOperaCache(bool dryRun = false);
    bool cleanBraveCache(bool dryRun = false);
    bool cleanVivaldiCache(bool dryRun = false);

    // Utility functions
    bool isAdmin() const;
    void showStatistics() const;
    std::string formatSize(uint64_t bytes) const;
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