#include "Cleaner.h"
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <shlobj.h>
#include <shellapi.h>
#include <iostream>
#include <filesystem>
#include <iomanip>
#include <sstream>
#include <thread>
#include <future>

// Initialize static member
Logger* Logger::instance = nullptr;

Cleaner::Cleaner() : tempStats(), recycleBinStats() {
    Logger::getInstance().log(LogLevel::INFO, "Cleaner initialized");
}

Cleaner::~Cleaner() {
    Logger::getInstance().log(LogLevel::INFO, "Cleaner destroyed");
}

void Cleaner::logError(const std::string& operation, const std::string& error) {
    std::string message = "Error in " + operation + ": " + error;
    Logger::getInstance().log(LogLevel::ERROR, message);
}

bool Cleaner::isPathExcluded(const std::wstring& path) const {
    for (const auto& excluded : excludedPaths) {
        if (path.find(excluded) != std::wstring::npos) {
            return true;
        }
    }
    return false;
}

bool Cleaner::isPathIncluded(const std::wstring& path) const {
    if (includedPaths.empty()) return true;
    for (const auto& included : includedPaths) {
        if (path.find(included) != std::wstring::npos) {
            return true;
        }
    }
    return false;
}

void Cleaner::setExcludedPaths(const std::vector<std::wstring>& paths) {
    excludedPaths = paths;
}

void Cleaner::setIncludedPaths(const std::vector<std::wstring>& paths) {
    includedPaths = paths;
}

std::string Cleaner::formatSize(uint64_t bytes) const {
    const char* units[] = {"B", "KB", "MB", "GB", "TB"};
    int unitIndex = 0;
    double size = static_cast<double>(bytes);

    while (size >= 1024 && unitIndex < 4) {
        size /= 1024;
        unitIndex++;
    }

    std::stringstream ss;
    ss << std::fixed << std::setprecision(2) << size << " " << units[unitIndex];
    return ss.str();
}

void Cleaner::showStatistics() const {
    Logger& logger = Logger::getInstance();
    logger.log(LogLevel::INFO, "Cleaning Statistics:");

    std::stringstream ss;
    ss << "Temporary Files:\n"
       << "  Files deleted: " << tempStats.filesDeleted << "\n"
       << "  Space freed: " << formatSize(tempStats.bytesFreed) << "\n"
       << "  Errors encountered: " << tempStats.errors;
    logger.log(LogLevel::INFO, ss.str());
    
    if (!tempStats.errorMessages.empty()) {
        logger.log(LogLevel::ERROR, "Temporary Files Error Details:");
        for (const auto& error : tempStats.errorMessages) {
            logger.log(LogLevel::ERROR, "  " + error);
        }
    }

    ss.str("");
    ss << "Recycle Bin:\n"
       << "  Files deleted: " << recycleBinStats.filesDeleted << "\n"
       << "  Space freed: " << formatSize(recycleBinStats.bytesFreed) << "\n"
       << "  Errors encountered: " << recycleBinStats.errors;
    logger.log(LogLevel::INFO, ss.str());

    if (!recycleBinStats.errorMessages.empty()) {
        logger.log(LogLevel::ERROR, "Recycle Bin Error Details:");
        for (const auto& error : recycleBinStats.errorMessages) {
            logger.log(LogLevel::ERROR, "  " + error);
        }
    }

    if (!browserStats.empty()) {
        logger.log(LogLevel::INFO, "Browser Cache:");
        for (const auto& stats : browserStats) {
            ss.str("");
            ss << "  " << stats.browserName << ":\n"
               << "    Files deleted: " << stats.filesDeleted << "\n"
               << "    Space freed: " << formatSize(stats.bytesFreed) << "\n"
               << "    Errors encountered: " << stats.errors;
            logger.log(LogLevel::INFO, ss.str());

            if (!stats.errorMessages.empty()) {
                logger.log(LogLevel::ERROR, stats.browserName + " Error Details:");
                for (const auto& error : stats.errorMessages) {
                    logger.log(LogLevel::ERROR, "  " + error);
                }
            }
        }
    }
}

bool Cleaner::cleanTempFiles(bool dryRun) {
    Logger::getInstance().log(LogLevel::INFO, "Starting temporary files cleanup" + std::string(dryRun ? " (dry run)" : ""));
    tempStats = TempFilesStats();
    auto directories = getTempDirectories();
    
    std::vector<std::future<void>> futures;
    
    for (const auto& dir : directories) {
        if (isPathExcluded(dir)) {
            Logger::getInstance().log(LogLevel::INFO, "Skipping excluded directory: " + std::string(dir.begin(), dir.end()));
            continue;
        }

        if (!isPathIncluded(dir)) {
            Logger::getInstance().log(LogLevel::INFO, "Skipping non-included directory: " + std::string(dir.begin(), dir.end()));
            continue;
        }

        futures.push_back(std::async(std::launch::async, [this, dir, dryRun]() {
            try {
                if (std::filesystem::exists(dir)) {
                    for (const auto& entry : std::filesystem::directory_iterator(dir)) {
                        try {
                            if (std::filesystem::is_regular_file(entry)) {
                                if (isPathExcluded(entry.path().wstring())) {
                                    continue;
                                }

                                uint64_t fileSize = std::filesystem::file_size(entry.path());
                                if (dryRun) {
                                    Logger::getInstance().log(LogLevel::INFO, 
                                        "Would delete: " + entry.path().string() + 
                                        " (" + formatSize(fileSize) + ")");
                                } else {
                                    if (std::filesystem::remove(entry.path())) {
                                        tempStats.filesDeleted++;
                                        tempStats.bytesFreed += fileSize;
                                    }
                                }
                            }
                        } catch (const std::exception& e) {
                            tempStats.errors++;
                            tempStats.errorMessages.push_back(
                                "Error processing " + entry.path().string() + ": " + e.what());
                            logError("cleanTempFiles", e.what());
                        }
                    }
                }
            } catch (const std::exception& e) {
                tempStats.errors++;
                tempStats.errorMessages.push_back(
                    "Error processing directory " + std::string(dir.begin(), dir.end()) + ": " + e.what());
                logError("cleanTempFiles", e.what());
            }
        }));
    }

    // Wait for all async operations to complete
    for (auto& future : futures) {
        future.wait();
    }

    Logger::getInstance().log(LogLevel::INFO, "Temporary files cleanup completed");
    return true;
}

bool Cleaner::cleanRecycleBin() {
    if (!isAdmin()) {
        std::cerr << "Administrator privileges required for recycle bin cleaning." << std::endl;
        return false;
    }

    // Get recycle bin info
    SHQUERYRBINFO rbInfo = {};
    rbInfo.cbSize = sizeof(SHQUERYRBINFO);
    
    if (SUCCEEDED(SHQueryRecycleBinW(NULL, &rbInfo))) {
        recycleBinStats.filesDeleted = static_cast<int>(rbInfo.i64NumItems);
        recycleBinStats.bytesFreed = rbInfo.i64Size;
    }

    // Empty recycle bin
    if (SUCCEEDED(SHEmptyRecycleBinW(NULL, NULL, 
        SHERB_NOCONFIRMATION | SHERB_NOPROGRESSUI | SHERB_NOSOUND))) {
        return true;
    }
    
    std::cerr << "Failed to empty recycle bin." << std::endl;
    return false;
}

bool Cleaner::deleteDirectory(const std::wstring& path) {
    try {
        if (std::filesystem::exists(path)) {
            std::filesystem::remove_all(path);
            return true;
        }
    } catch (const std::exception& e) {
        std::wcerr << L"Error deleting directory" << std::endl;
    }
    return false;
}

std::vector<std::wstring> Cleaner::getTempDirectories() const {
    std::vector<std::wstring> directories;
    
    // Get %TEMP% directory
    wchar_t tempPath[MAX_PATH];
    if (GetTempPathW(MAX_PATH, tempPath)) {
        directories.push_back(tempPath);
    }
    
    // Get %LOCALAPPDATA%\Temp directory
    wchar_t localAppDataPath[MAX_PATH];
    if (SUCCEEDED(SHGetFolderPathW(NULL, CSIDL_LOCAL_APPDATA, NULL, 0, localAppDataPath))) {
        std::wstring localTempPath = std::wstring(localAppDataPath) + L"\\Temp";
        directories.push_back(localTempPath);
    }
    
    return directories;
}

bool Cleaner::cleanBrowserCache() {
    bool success = true;
    browserStats.clear();
    
    if (!cleanChromiumCache()) {
        success = false;
    }
    
    if (!cleanFirefoxCache()) {
        success = false;
    }
    
    return success;
}

bool Cleaner::cleanChromiumCache() {
    const std::wstring localAppData = getLocalAppData();
    if (localAppData.empty()) return false;

    // Chrome cache paths
    std::vector<std::wstring> chromePaths = {
        localAppData + L"\\Google\\Chrome\\User Data\\Default\\Cache",
        localAppData + L"\\Google\\Chrome\\User Data\\Default\\Code Cache",
        localAppData + L"\\Google\\Chrome\\User Data\\Default\\GPUCache"
    };

    // Edge cache paths
    std::vector<std::wstring> edgePaths = {
        localAppData + L"\\Microsoft\\Edge\\User Data\\Default\\Cache",
        localAppData + L"\\Microsoft\\Edge\\User Data\\Default\\Code Cache",
        localAppData + L"\\Microsoft\\Edge\\User Data\\Default\\GPUCache"
    };

    BrowserCacheStats chromeStats;
    chromeStats.browserName = "Google Chrome";
    
    // Clean Chrome cache
    for (const auto& path : chromePaths) {
        try {
            if (std::filesystem::exists(path)) {
                for (const auto& entry : std::filesystem::directory_iterator(path)) {
                    try {
                        if (std::filesystem::is_regular_file(entry)) {
                            uint64_t fileSize = std::filesystem::file_size(entry.path());
                            if (std::filesystem::remove(entry.path())) {
                                chromeStats.filesDeleted++;
                                chromeStats.bytesFreed += fileSize;
                            }
                        }
                    } catch (const std::exception& e) {
                        chromeStats.errors++;
                    }
                }
            }
        } catch (const std::exception& e) {
            chromeStats.errors++;
        }
    }
    
    BrowserCacheStats edgeStats;
    edgeStats.browserName = "Microsoft Edge";
    
    // Clean Edge cache
    for (const auto& path : edgePaths) {
        try {
            if (std::filesystem::exists(path)) {
                for (const auto& entry : std::filesystem::directory_iterator(path)) {
                    try {
                        if (std::filesystem::is_regular_file(entry)) {
                            uint64_t fileSize = std::filesystem::file_size(entry.path());
                            if (std::filesystem::remove(entry.path())) {
                                edgeStats.filesDeleted++;
                                edgeStats.bytesFreed += fileSize;
                            }
                        }
                    } catch (const std::exception& e) {
                        edgeStats.errors++;
                    }
                }
            }
        } catch (const std::exception& e) {
            edgeStats.errors++;
        }
    }

    browserStats.push_back(chromeStats);
    browserStats.push_back(edgeStats);
    
    return true;
}

bool Cleaner::cleanFirefoxCache() {
    const std::wstring localAppData = getLocalAppData();
    if (localAppData.empty()) return false;

    // Firefox cache paths
    std::vector<std::wstring> firefoxPaths = {
        localAppData + L"\\Mozilla\\Firefox\\Profiles"
    };

    BrowserCacheStats firefoxStats;
    firefoxStats.browserName = "Mozilla Firefox";

    try {
        for (const auto& basePath : firefoxPaths) {
            if (!std::filesystem::exists(basePath)) continue;

            // Iterate through profile directories
            for (const auto& profile : std::filesystem::directory_iterator(basePath)) {
                if (!std::filesystem::is_directory(profile)) continue;

                // Cache2 directory contains the browser cache
                std::wstring cachePath = profile.path() / L"cache2";
                if (!std::filesystem::exists(cachePath)) continue;

                // Clean cache files
                for (const auto& entry : std::filesystem::recursive_directory_iterator(cachePath)) {
                    try {
                        if (std::filesystem::is_regular_file(entry)) {
                            uint64_t fileSize = std::filesystem::file_size(entry.path());
                            if (std::filesystem::remove(entry.path())) {
                                firefoxStats.filesDeleted++;
                                firefoxStats.bytesFreed += fileSize;
                            }
                        }
                    } catch (const std::exception& e) {
                        firefoxStats.errors++;
                    }
                }
            }
        }
    } catch (const std::exception& e) {
        firefoxStats.errors++;
    }

    browserStats.push_back(firefoxStats);
    return true;
}

bool Cleaner::cleanRegistry() {
    // TODO: Implement registry cleaning
    return true;
}

bool Cleaner::isAdmin() const {
    BOOL isAdmin = FALSE;
    SID_IDENTIFIER_AUTHORITY NtAuthority = SECURITY_NT_AUTHORITY;
    PSID AdministratorsGroup;
    
    if (AllocateAndInitializeSid(&NtAuthority, 2, SECURITY_BUILTIN_DOMAIN_RID, DOMAIN_ALIAS_RID_ADMINS, 0, 0, 0, 0, 0, 0, &AdministratorsGroup)) {
        if (!CheckTokenMembership(NULL, AdministratorsGroup, &isAdmin)) {
            isAdmin = FALSE;
        }
        FreeSid(AdministratorsGroup);
    }
    
    return isAdmin == TRUE;
}

bool Cleaner::deleteFile(const std::string& path) {
    try {
        if (std::filesystem::exists(path)) {
            if (std::filesystem::remove(path)) {
                tempStats.filesDeleted++;
                return true;
            }
        }
    } catch (const std::exception& e) {
        tempStats.errors++;
    }
    return false;
}

std::wstring Cleaner::getLocalAppData() const {
    wchar_t path[MAX_PATH];
    if (SUCCEEDED(SHGetFolderPathW(NULL, CSIDL_LOCAL_APPDATA, NULL, 0, path))) {
        return std::wstring(path);
    }
    return L"";
} 