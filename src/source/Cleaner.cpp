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
#include <chrono>
#include <fstream>
#include <algorithm>
#include <regex>
#include <mutex>

// Initialize static members
std::unique_ptr<Logger> Logger::instance = nullptr;
std::mutex Logger::mutex;

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
    logger.log(LogLevel::INFO, "=== Cleaning Statistics ===");

    // Temp files stats
    logger.log(LogLevel::INFO, "Temporary Files:");
    logger.log(LogLevel::INFO, "  Files deleted: " + std::to_string(tempStats.filesDeleted));
    logger.log(LogLevel::INFO, "  Space freed: " + formatSize(tempStats.bytesFreed));
    logger.log(LogLevel::INFO, "  Errors: " + std::to_string(tempStats.errors));
    
    if (!tempStats.errorMessages.empty()) {
        logger.log(LogLevel::ERROR, "Temporary Files Error Details:");
        for (const auto& error : tempStats.errorMessages) {
            logger.log(LogLevel::ERROR, "  " + error);
        }
    }

    // Browser cache stats
    logger.log(LogLevel::INFO, "Browser Cache:");
    for (const auto& stats : browserStats) {
        logger.log(LogLevel::INFO, "  " + stats.browserName + ":");
        logger.log(LogLevel::INFO, "    Files deleted: " + std::to_string(stats.filesDeleted));
        logger.log(LogLevel::INFO, "    Space freed: " + formatSize(stats.bytesFreed));
        logger.log(LogLevel::INFO, "    Errors: " + std::to_string(stats.errors));

        if (!stats.errorMessages.empty()) {
            logger.log(LogLevel::ERROR, stats.browserName + " Error Details:");
            for (const auto& error : stats.errorMessages) {
                logger.log(LogLevel::ERROR, "  " + error);
            }
        }
    }

    // Recycle bin stats
    logger.log(LogLevel::INFO, "Recycle Bin:");
    logger.log(LogLevel::INFO, "  Files deleted: " + std::to_string(recycleBinStats.filesDeleted));
    logger.log(LogLevel::INFO, "  Space freed: " + formatSize(recycleBinStats.bytesFreed));
    logger.log(LogLevel::INFO, "  Errors: " + std::to_string(recycleBinStats.errors));

    // Registry stats
    logger.log(LogLevel::INFO, "Registry:");
    logger.log(LogLevel::INFO, "  Keys deleted: " + std::to_string(registryStats.keysDeleted));
    logger.log(LogLevel::INFO, "  Values deleted: " + std::to_string(registryStats.valuesDeleted));
    logger.log(LogLevel::INFO, "  Errors: " + std::to_string(registryStats.errors));
}

bool Cleaner::cleanTempFiles(bool dryRun) {
    Logger::getInstance().log(LogLevel::INFO, "Starting temporary files cleaning" + std::string(dryRun ? " (dry run)" : ""));
    
    tempStats = TempFilesStats();
    auto tempDirs = getTempDirectories();
    
    for (const auto& dir : tempDirs) {
        if (!isPathIncluded(dir) || isPathExcluded(dir)) {
            continue;
        }
        
        try {
            for (const auto& entry : std::filesystem::recursive_directory_iterator(dir)) {
                if (entry.is_regular_file()) {
                    std::string path = entry.path().string();
                    if (deleteFile(path, dryRun)) {
                        tempStats.filesDeleted++;
                        tempStats.bytesFreed += entry.file_size();
                    }
                }
            }
        } catch (const std::exception& e) {
            std::string error = "Error processing directory " + std::string(dir.begin(), dir.end()) + ": " + e.what();
            logError("cleanTempFiles", error);
            tempStats.errors++;
            tempStats.errorMessages.push_back(error);
        }
    }
    
    Logger::getInstance().log(LogLevel::INFO, 
        "Temporary files cleaning completed: " + 
        std::to_string(tempStats.filesDeleted) + " files deleted, " +
        formatSize(tempStats.bytesFreed) + " freed");
    
    return tempStats.errors == 0;
}

bool Cleaner::cleanRecycleBin(bool dryRun) {
    Logger::getInstance().log(LogLevel::INFO, "Starting recycle bin cleaning" + std::string(dryRun ? " (dry run)" : ""));
    
    recycleBinStats = RecycleBinStats();
    
    if (dryRun) {
        Logger::getInstance().log(LogLevel::INFO, "Dry run: would empty recycle bin");
        return true;
    }
    
    SHEmptyRecycleBinW(nullptr, nullptr, SHERB_NOCONFIRMATION | SHERB_NOPROGRESSUI | SHERB_NOSOUND);
    
    // Note: We can't get exact statistics for recycle bin
    Logger::getInstance().log(LogLevel::INFO, "Recycle bin emptied");
    return true;
}

bool Cleaner::cleanBrowserCache(bool dryRun) {
    Logger::getInstance().log(LogLevel::INFO, "Starting browser cache cleaning" + std::string(dryRun ? " (dry run)" : ""));
    
    browserStats.clear();
    bool success = true;
    
    // Clean Chromium-based browsers
    success &= cleanChromiumCache(dryRun);
    success &= cleanBraveCache(dryRun);
    success &= cleanVivaldiCache(dryRun);
    success &= cleanOperaCache(dryRun);
    
    // Clean Firefox
    success &= cleanFirefoxCache(dryRun);
    
    return success;
}

bool Cleaner::cleanRegistry(bool dryRun) {
    Logger::getInstance().log(LogLevel::INFO, "Starting registry cleaning" + std::string(dryRun ? " (dry run)" : ""));
    
    registryStats = RegistryStats();
    auto obsoleteKeys = getObsoleteRegistryKeys();
    
    for (const auto& key : obsoleteKeys) {
        if (cleanRegistryKey(HKEY_CURRENT_USER, key, dryRun)) {
            registryStats.keysDeleted++;
        } else {
            registryStats.errors++;
        }
    }
    
    Logger::getInstance().log(LogLevel::INFO, 
        "Registry cleaning completed: " + 
        std::to_string(registryStats.keysDeleted) + " keys deleted");
    
    return registryStats.errors == 0;
}

bool Cleaner::isAdmin() const {
    BOOL isAdmin = FALSE;
    PSID adminGroup = nullptr;
    SID_IDENTIFIER_AUTHORITY NtAuthority = SECURITY_NT_AUTHORITY;
    
    if (AllocateAndInitializeSid(&NtAuthority, 2, SECURITY_BUILTIN_DOMAIN_RID, DOMAIN_ALIAS_RID_ADMINS,
                                0, 0, 0, 0, 0, 0, &adminGroup)) {
        if (!CheckTokenMembership(nullptr, adminGroup, &isAdmin)) {
            isAdmin = FALSE;
        }
        FreeSid(adminGroup);
    }
    
    return isAdmin != FALSE;
}

std::vector<std::wstring> Cleaner::getTempDirectories() const {
    std::vector<std::wstring> tempDirs;
    
    // System temp directory
    wchar_t systemTemp[MAX_PATH];
    if (GetTempPathW(MAX_PATH, systemTemp) > 0) {
        tempDirs.push_back(systemTemp);
    }
    
    // User temp directory
    wchar_t userTemp[MAX_PATH];
    if (SUCCEEDED(SHGetFolderPathW(nullptr, CSIDL_LOCAL_APPDATA, nullptr, 0, userTemp))) {
        std::wstring userTempPath(userTemp);
        userTempPath += L"\\Temp";
        tempDirs.push_back(userTempPath);
    }
    
    return tempDirs;
}

bool Cleaner::deleteFile(const std::string& path, bool dryRun) {
    try {
        if (dryRun) {
            Logger::getInstance().log(LogLevel::INFO, "Dry run: would delete " + path);
            return true;
        }
        
        if (std::filesystem::remove(path)) {
            Logger::getInstance().log(LogLevel::INFO, "Deleted: " + path);
            return true;
        }
    } catch (const std::exception& e) {
        logError("deleteFile", "Error deleting " + path + ": " + e.what());
    }
    return false;
}

bool Cleaner::cleanChromiumCache(bool dryRun) {
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
                            if (dryRun) {
                                Logger::getInstance().log(LogLevel::INFO, 
                                    "Would delete: " + entry.path().string() + 
                                    " (" + formatSize(fileSize) + ")");
                            } else {
                                if (std::filesystem::remove(entry.path())) {
                                    chromeStats.filesDeleted++;
                                    chromeStats.bytesFreed += fileSize;
                                }
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
                            if (dryRun) {
                                Logger::getInstance().log(LogLevel::INFO, 
                                    "Would delete: " + entry.path().string() + 
                                    " (" + formatSize(fileSize) + ")");
                            } else {
                                if (std::filesystem::remove(entry.path())) {
                                    edgeStats.filesDeleted++;
                                    edgeStats.bytesFreed += fileSize;
                                }
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

bool Cleaner::cleanFirefoxCache(bool dryRun) {
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
                            if (dryRun) {
                                Logger::getInstance().log(LogLevel::INFO, 
                                    "Would delete: " + entry.path().string() + 
                                    " (" + formatSize(fileSize) + ")");
                            } else {
                                if (std::filesystem::remove(entry.path())) {
                                    firefoxStats.filesDeleted++;
                                    firefoxStats.bytesFreed += fileSize;
                                }
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

bool Cleaner::cleanOperaCache(bool dryRun) {
    const std::wstring localAppData = getLocalAppData();
    if (localAppData.empty()) return false;

    BrowserCacheStats operaStats;
    operaStats.browserName = "Opera";
    
    std::wstring operaPath = localAppData + L"\\Opera Software\\Opera Stable\\Cache";
    if (!std::filesystem::exists(operaPath)) {
        Logger::getInstance().log(LogLevel::WARNING, "Opera cache directory not found");
        return false;
    }

    try {
        for (const auto& entry : std::filesystem::recursive_directory_iterator(operaPath)) {
            if (entry.is_regular_file()) {
                uint64_t fileSize = entry.file_size();
                if (dryRun) {
                    Logger::getInstance().log(LogLevel::INFO, 
                        "Would delete: " + entry.path().string() + 
                        " (" + formatSize(fileSize) + ")");
                } else {
                    if (std::filesystem::remove(entry.path())) {
                        operaStats.filesDeleted++;
                        operaStats.bytesFreed += fileSize;
                    }
                }
            }
        }
    } catch (const std::exception& e) {
        std::string error = "Error cleaning Opera cache: " + std::string(e.what());
        logError("cleanOperaCache", error);
        operaStats.errors++;
        operaStats.errorMessages.push_back(error);
    }

    browserStats.push_back(operaStats);
    return operaStats.errors == 0;
}

bool Cleaner::cleanBraveCache(bool dryRun) {
    const std::wstring localAppData = getLocalAppData();
    if (localAppData.empty()) return false;

    BrowserCacheStats braveStats;
    braveStats.browserName = "Brave";
    
    std::wstring bravePath = localAppData + L"\\BraveSoftware\\Brave-Browser\\User Data\\Default\\Cache";
    if (!std::filesystem::exists(bravePath)) {
        Logger::getInstance().log(LogLevel::WARNING, "Brave cache directory not found");
        return false;
    }

    try {
        for (const auto& entry : std::filesystem::recursive_directory_iterator(bravePath)) {
            if (entry.is_regular_file()) {
                uint64_t fileSize = entry.file_size();
                if (dryRun) {
                    Logger::getInstance().log(LogLevel::INFO, 
                        "Would delete: " + entry.path().string() + 
                        " (" + formatSize(fileSize) + ")");
                } else {
                    if (std::filesystem::remove(entry.path())) {
                        braveStats.filesDeleted++;
                        braveStats.bytesFreed += fileSize;
                    }
                }
            }
        }
    } catch (const std::exception& e) {
        std::string error = "Error cleaning Brave cache: " + std::string(e.what());
        logError("cleanBraveCache", error);
        braveStats.errors++;
        braveStats.errorMessages.push_back(error);
    }

    browserStats.push_back(braveStats);
    return braveStats.errors == 0;
}

bool Cleaner::cleanVivaldiCache(bool dryRun) {
    const std::wstring localAppData = getLocalAppData();
    if (localAppData.empty()) return false;

    BrowserCacheStats vivaldiStats;
    vivaldiStats.browserName = "Vivaldi";
    
    std::wstring vivaldiPath = localAppData + L"\\Vivaldi\\User Data\\Default\\Cache";
    if (!std::filesystem::exists(vivaldiPath)) {
        Logger::getInstance().log(LogLevel::WARNING, "Vivaldi cache directory not found");
        return false;
    }

    try {
        for (const auto& entry : std::filesystem::recursive_directory_iterator(vivaldiPath)) {
            if (entry.is_regular_file()) {
                uint64_t fileSize = entry.file_size();
                if (dryRun) {
                    Logger::getInstance().log(LogLevel::INFO, 
                        "Would delete: " + entry.path().string() + 
                        " (" + formatSize(fileSize) + ")");
                } else {
                    if (std::filesystem::remove(entry.path())) {
                        vivaldiStats.filesDeleted++;
                        vivaldiStats.bytesFreed += fileSize;
                    }
                }
            }
        }
    } catch (const std::exception& e) {
        std::string error = "Error cleaning Vivaldi cache: " + std::string(e.what());
        logError("cleanVivaldiCache", error);
        vivaldiStats.errors++;
        vivaldiStats.errorMessages.push_back(error);
    }

    browserStats.push_back(vivaldiStats);
    return vivaldiStats.errors == 0;
}

bool Cleaner::cleanRegistryKey(HKEY hKey, const std::wstring& subKey, bool dryRun) {
    if (dryRun) {
        Logger::getInstance().log(LogLevel::INFO, "Dry run: would clean registry key " + std::string(subKey.begin(), subKey.end()));
        return true;
    }

    HKEY hSubKey;
    if (RegOpenKeyExW(hKey, subKey.c_str(), 0, KEY_ALL_ACCESS, &hSubKey) != ERROR_SUCCESS) {
        std::string error = "Failed to open registry key: " + std::string(subKey.begin(), subKey.end());
        logError("cleanRegistryKey", error);
        registryStats.errors++;
        registryStats.errorMessages.push_back(error);
        return false;
    }

    // Delete all values
    DWORD valueCount;
    if (RegQueryInfoKeyW(hSubKey, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, &valueCount, nullptr, nullptr, nullptr, nullptr) == ERROR_SUCCESS) {
        for (DWORD i = 0; i < valueCount; i++) {
            wchar_t valueName[MAX_PATH];
            DWORD valueNameSize = MAX_PATH;
            if (RegEnumValueW(hSubKey, i, valueName, &valueNameSize, nullptr, nullptr, nullptr, nullptr) == ERROR_SUCCESS) {
                if (deleteRegistryValue(hKey, subKey, valueName, dryRun)) {
                    registryStats.valuesDeleted++;
                }
            }
        }
    }

    // Delete all subkeys
    DWORD subKeyCount;
    if (RegQueryInfoKeyW(hSubKey, nullptr, nullptr, nullptr, &subKeyCount, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr) == ERROR_SUCCESS) {
        for (DWORD i = 0; i < subKeyCount; i++) {
            wchar_t subKeyName[MAX_PATH];
            DWORD subKeyNameSize = MAX_PATH;
            if (RegEnumKeyExW(hSubKey, i, subKeyName, &subKeyNameSize, nullptr, nullptr, nullptr, nullptr) == ERROR_SUCCESS) {
                std::wstring fullSubKey = subKey + L"\\" + subKeyName;
                if (cleanRegistryKey(hKey, fullSubKey, dryRun)) {
                    registryStats.keysDeleted++;
                }
            }
        }
    }

    RegCloseKey(hSubKey);
    return true;
}

bool Cleaner::deleteRegistryValue(HKEY hKey, const std::wstring& subKey, const std::wstring& valueName, bool dryRun) {
    if (dryRun) {
        Logger::getInstance().log(LogLevel::INFO, 
            "Dry run: would delete registry value " + 
            std::string(subKey.begin(), subKey.end()) + "\\" + 
            std::string(valueName.begin(), valueName.end()));
        return true;
    }

    HKEY hSubKey;
    if (RegOpenKeyExW(hKey, subKey.c_str(), 0, KEY_ALL_ACCESS, &hSubKey) != ERROR_SUCCESS) {
        return false;
    }

    LONG result = RegDeleteValueW(hSubKey, valueName.c_str());
    RegCloseKey(hSubKey);

    if (result != ERROR_SUCCESS) {
        std::string error = "Failed to delete registry value: " + 
            std::string(subKey.begin(), subKey.end()) + "\\" + 
            std::string(valueName.begin(), valueName.end());
        logError("deleteRegistryValue", error);
        registryStats.errors++;
        registryStats.errorMessages.push_back(error);
        return false;
    }

    return true;
}

std::vector<std::wstring> Cleaner::getObsoleteRegistryKeys() const {
    std::vector<std::wstring> keys;
    
    // Add known obsolete registry keys
    keys.push_back(L"Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\RecentDocs");
    keys.push_back(L"Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\RunMRU");
    keys.push_back(L"Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\TypedPaths");
    keys.push_back(L"Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\ComDlg32\\OpenSaveMRU");
    keys.push_back(L"Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\ComDlg32\\LastVisitedMRU");
    
    return keys;
}

std::wstring Cleaner::getLocalAppData() const {
    wchar_t localAppData[MAX_PATH];
    if (SUCCEEDED(SHGetFolderPathW(nullptr, CSIDL_LOCAL_APPDATA, nullptr, 0, localAppData))) {
        return localAppData;
    }
    return L"";
}

std::string Cleaner::generateBackupPath(const std::string& operationType) const {
    auto now = std::chrono::system_clock::now();
    auto time = std::chrono::system_clock::to_time_t(now);
    std::stringstream ss;
    ss << "backups\\" << operationType << "_" << std::put_time(std::localtime(&time), "%Y%m%d_%H%M%S");
    return ss.str();
}

bool Cleaner::createBackup(const std::string& operationType) {
    Logger::getInstance().log(LogLevel::INFO, "Creating backup for operation: " + operationType);
    
    BackupInfo backup;
    backup.operationType = operationType;
    backup.timestamp = std::to_string(std::chrono::system_clock::now().time_since_epoch().count());
    backup.backupPath = generateBackupPath(operationType);
    
    // Create backup directory
    std::filesystem::create_directories(backup.backupPath);
    
    bool success = true;
    
    if (operationType == "temp") {
        auto tempDirs = getTempDirectories();
        for (const auto& dir : tempDirs) {
            if (!isPathIncluded(dir) || isPathExcluded(dir)) {
                continue;
            }
            
            try {
                for (const auto& entry : std::filesystem::recursive_directory_iterator(dir)) {
                    if (entry.is_regular_file()) {
                        std::string sourcePath = entry.path().string();
                        std::string targetPath = backup.backupPath + "\\" + entry.path().filename().string();
                        if (backupFile(sourcePath, targetPath)) {
                            backup.files.push_back(sourcePath);
                            backup.totalSize += entry.file_size();
                        }
                    }
                }
            } catch (const std::exception& e) {
                std::string error = "Error backing up directory " + std::string(dir.begin(), dir.end()) + ": " + e.what();
                logError("createBackup", error);
                success = false;
            }
        }
    } else if (operationType == "registry") {
        auto obsoleteKeys = getObsoleteRegistryKeys();
        for (const auto& key : obsoleteKeys) {
            if (backupRegistryKey(HKEY_CURRENT_USER, key, backup.backupPath)) {
                backup.registryKeys.push_back({std::string(key.begin(), key.end()), ""});
            }
        }
    } else if (operationType == "browser") {
        auto browserPaths = getBrowserPaths();
        for (const auto& path : browserPaths) {
            try {
                for (const auto& entry : std::filesystem::recursive_directory_iterator(path)) {
                    if (entry.is_regular_file()) {
                        std::string sourcePath = entry.path().string();
                        std::string targetPath = backup.backupPath + "\\" + entry.path().filename().string();
                        if (backupFile(sourcePath, targetPath)) {
                            backup.files.push_back(sourcePath);
                            backup.totalSize += entry.file_size();
                        }
                    }
                }
            } catch (const std::exception& e) {
                std::string error = "Error backing up browser path " + std::string(path.begin(), path.end()) + ": " + e.what();
                logError("createBackup", error);
                success = false;
            }
        }
    }
    
    if (success) {
        backupHistory.push_back(backup);
        Logger::getInstance().log(LogLevel::INFO, "Backup created successfully: " + backup.backupPath);
    } else {
        Logger::getInstance().log(LogLevel::ERROR, "Backup creation completed with errors");
    }
    
    return success;
}

bool Cleaner::backupFile(const std::string& sourcePath, const std::string& backupPath) {
    try {
        std::filesystem::copy_file(sourcePath, backupPath, std::filesystem::copy_options::overwrite_existing);
        return true;
    } catch (const std::exception& e) {
        std::string error = "Error backing up file " + sourcePath + ": " + e.what();
        logError("backupFile", error);
        return false;
    }
}

bool Cleaner::backupRegistryKey(HKEY hKey, const std::wstring& subKey, const std::string& backupPath) {
    HKEY hSubKey;
    if (RegOpenKeyExW(hKey, subKey.c_str(), 0, KEY_READ, &hSubKey) != ERROR_SUCCESS) {
        return false;
    }
    
    // Create backup file
    std::string backupFile = backupPath + "\\" + std::string(subKey.begin(), subKey.end()) + ".reg";
    std::ofstream file(backupFile);
    if (!file.is_open()) {
        RegCloseKey(hSubKey);
        return false;
    }
    
    // Write registry key to file
    file << "Windows Registry Editor Version 5.00\n\n";
    file << "[" << std::string(subKey.begin(), subKey.end()) << "]\n";
    
    // Get and write values
    DWORD valueCount;
    if (RegQueryInfoKeyW(hSubKey, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, &valueCount, nullptr, nullptr, nullptr, nullptr) == ERROR_SUCCESS) {
        for (DWORD i = 0; i < valueCount; i++) {
            wchar_t valueName[MAX_PATH];
            DWORD valueNameSize = MAX_PATH;
            DWORD valueType;
            BYTE valueData[4096];
            DWORD valueDataSize = sizeof(valueData);
            
            if (RegEnumValueW(hSubKey, i, valueName, &valueNameSize, nullptr, &valueType, valueData, &valueDataSize) == ERROR_SUCCESS) {
                file << "\"" << std::string(valueName, valueName + valueNameSize) << "\"=";
                
                switch (valueType) {
                    case REG_SZ:
                        file << "\"" << std::string(reinterpret_cast<char*>(valueData), valueDataSize - 1) << "\"";
                        break;
                    case REG_DWORD:
                        file << "dword:" << std::hex << *reinterpret_cast<DWORD*>(valueData);
                        break;
                    // Add more value types as needed
                }
                
                file << "\n";
            }
        }
    }
    
    RegCloseKey(hSubKey);
    return true;
}

bool Cleaner::restoreFromBackup(const std::string& backupPath) {
    Logger::getInstance().log(LogLevel::INFO, "Restoring from backup: " + backupPath);
    
    // Find backup in history
    auto it = std::find_if(backupHistory.begin(), backupHistory.end(),
        [&backupPath](const BackupInfo& backup) { return backup.backupPath == backupPath; });
    
    if (it == backupHistory.end()) {
        Logger::getInstance().log(LogLevel::ERROR, "Backup not found in history: " + backupPath);
        return false;
    }
    
    const BackupInfo& backup = *it;
    bool success = true;
    
    if (backup.operationType == "temp" || backup.operationType == "browser") {
        for (const auto& file : backup.files) {
            std::string targetPath = file;
            if (!restoreFile(backupPath + "\\" + std::filesystem::path(file).filename().string(), targetPath)) {
                success = false;
            }
        }
    } else if (backup.operationType == "registry") {
        for (const auto& key : backup.registryKeys) {
            if (!restoreRegistryKey(backupPath, HKEY_CURRENT_USER, std::wstring(key.first.begin(), key.first.end()))) {
                success = false;
            }
        }
    }
    
    if (success) {
        Logger::getInstance().log(LogLevel::INFO, "Backup restored successfully");
    } else {
        Logger::getInstance().log(LogLevel::ERROR, "Backup restoration completed with errors");
    }
    
    return success;
}

bool Cleaner::restoreFile(const std::string& backupPath, const std::string& targetPath) {
    try {
        std::filesystem::copy_file(backupPath, targetPath, std::filesystem::copy_options::overwrite_existing);
        return true;
    } catch (const std::exception& e) {
        std::string error = "Error restoring file " + targetPath + ": " + e.what();
        logError("restoreFile", error);
        return false;
    }
}

bool Cleaner::restoreRegistryKey(const std::string& backupPath, HKEY hKey, const std::wstring& subKey) {
    std::string backupFile = backupPath + "\\" + std::string(subKey.begin(), subKey.end()) + ".reg";
    if (!std::filesystem::exists(backupFile)) {
        return false;
    }
    
    // Use reg.exe to import the backup file
    std::string command = "reg import \"" + backupFile + "\"";
    int result = system(command.c_str());
    return result == 0;
}

std::vector<BackupInfo> Cleaner::getAvailableBackups() const {
    return backupHistory;
}

bool Cleaner::deleteBackup(const std::string& backupPath) {
    Logger::getInstance().log(LogLevel::INFO, "Deleting backup: " + backupPath);
    
    // Find backup in history
    auto it = std::find_if(backupHistory.begin(), backupHistory.end(),
        [&backupPath](const BackupInfo& backup) { return backup.backupPath == backupPath; });
    
    if (it == backupHistory.end()) {
        Logger::getInstance().log(LogLevel::ERROR, "Backup not found in history: " + backupPath);
        return false;
    }
    
    try {
        if (std::filesystem::remove_all(backupPath)) {
            backupHistory.erase(it);
            Logger::getInstance().log(LogLevel::INFO, "Backup deleted successfully");
            return true;
        }
    } catch (const std::exception& e) {
        std::string error = "Error deleting backup: " + std::string(e.what());
        logError("deleteBackup", error);
    }
    
    return false;
}

// Implementation of backup-specific cleaning functions
bool Cleaner::cleanTempFilesWithBackup(bool dryRun) {
    Logger::getInstance().log(LogLevel::INFO, "Starting temporary files cleaning with backup" + std::string(dryRun ? " (dry run)" : ""));
    
    // Create backup first
    if (!dryRun && !createBackup("temp")) {
        Logger::getInstance().log(LogLevel::ERROR, "Failed to create backup before cleaning");
        return false;
    }
    
    // Perform cleaning
    return cleanTempFiles(dryRun);
}

bool Cleaner::cleanRegistryWithBackup(bool dryRun) {
    Logger::getInstance().log(LogLevel::INFO, "Starting registry cleaning with backup" + std::string(dryRun ? " (dry run)" : ""));
    
    // Create backup first
    if (!dryRun && !createBackup("registry")) {
        Logger::getInstance().log(LogLevel::ERROR, "Failed to create backup before cleaning");
        return false;
    }
    
    // Perform cleaning
    return cleanRegistry(dryRun);
}

bool Cleaner::cleanBrowserCacheWithBackup(bool dryRun) {
    Logger::getInstance().log(LogLevel::INFO, "Starting browser cache cleaning with backup" + std::string(dryRun ? " (dry run)" : ""));
    
    // Create backup first
    if (!dryRun && !createBackup("browser")) {
        Logger::getInstance().log(LogLevel::ERROR, "Failed to create backup before cleaning");
        return false;
    }
    
    // Perform cleaning
    return cleanBrowserCache(dryRun);
}

bool Cleaner::cleanRecycleBinWithBackup(bool dryRun) {
    Logger::getInstance().log(LogLevel::INFO, "Starting recycle bin cleaning with backup" + std::string(dryRun ? " (dry run)" : ""));
    
    // Note: We can't create a backup of recycle bin contents
    // as they are already deleted files
    return cleanRecycleBin(dryRun);
}

bool Cleaner::deleteDirectory(const std::wstring& path, bool dryRun) {
    try {
        if (dryRun) {
            Logger::getInstance().log(LogLevel::INFO, "Dry run: would delete directory " + std::string(path.begin(), path.end()));
            return true;
        }
        
        if (std::filesystem::remove_all(path)) {
            Logger::getInstance().log(LogLevel::INFO, "Deleted directory: " + std::string(path.begin(), path.end()));
            return true;
        }
    } catch (const std::exception& e) {
        std::string error = "Error deleting directory " + std::string(path.begin(), path.end()) + ": " + e.what();
        logError("deleteDirectory", error);
    }
    return false;
}

std::vector<std::wstring> Cleaner::getBrowserPaths() const {
    std::vector<std::wstring> paths;
    const std::wstring localAppData = getLocalAppData();
    
    if (!localAppData.empty()) {
        // Chrome
        paths.push_back(localAppData + L"\\Google\\Chrome\\User Data\\Default\\Cache");
        
        // Edge
        paths.push_back(localAppData + L"\\Microsoft\\Edge\\User Data\\Default\\Cache");
        
        // Firefox
        paths.push_back(localAppData + L"\\Mozilla\\Firefox\\Profiles");
        
        // Opera
        paths.push_back(localAppData + L"\\Opera Software\\Opera Stable\\Cache");
        
        // Brave
        paths.push_back(localAppData + L"\\BraveSoftware\\Brave-Browser\\User Data\\Default\\Cache");
        
        // Vivaldi
        paths.push_back(localAppData + L"\\Vivaldi\\User Data\\Default\\Cache");
    }
    
    return paths;
} 