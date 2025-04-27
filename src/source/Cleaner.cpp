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
    if (!isAdmin()) {
        Logger::getInstance().log(LogLevel::ERROR, "Administrator privileges required for registry cleaning");
        return false;
    }

    Logger::getInstance().log(LogLevel::INFO, "Starting registry cleanup");
    
    registryStats = RegistryStats();
    bool success = true;
    
    // Очистка для текущего пользователя
    for (const auto& key : getObsoleteRegistryKeys()) {
        if (!cleanRegistryKey(HKEY_CURRENT_USER, key, false)) {
            success = false;
        }
    }
    
    // Очистка для всей системы
    for (const auto& key : getObsoleteRegistryKeys()) {
        if (!cleanRegistryKey(HKEY_LOCAL_MACHINE, key, false)) {
            success = false;
        }
    }

    // Добавляем статистику в лог
    std::stringstream ss;
    ss << "Registry cleanup completed:\n"
       << "  Keys deleted: " << registryStats.keysDeleted << "\n"
       << "  Values deleted: " << registryStats.valuesDeleted << "\n"
       << "  Errors encountered: " << registryStats.errors;
    Logger::getInstance().log(LogLevel::INFO, ss.str());

    if (!registryStats.errorMessages.empty()) {
        Logger::getInstance().log(LogLevel::ERROR, "Registry Cleanup Error Details:");
        for (const auto& error : registryStats.errorMessages) {
            Logger::getInstance().log(LogLevel::ERROR, "  " + error);
        }
    }

    return success;
}

bool Cleaner::cleanRegistryKey(HKEY hKey, const std::wstring& subKey, bool dryRun) {
    HKEY hSubKey;
    LONG result = RegOpenKeyExW(hKey, subKey.c_str(), 0, KEY_READ | KEY_WRITE, &hSubKey);
    
    if (result != ERROR_SUCCESS) {
        if (result != ERROR_FILE_NOT_FOUND) {
            registryStats.errors++;
            registryStats.errorMessages.push_back(
                "Failed to open key: " + std::string(subKey.begin(), subKey.end()));
        }
        return false;
    }

    // Получаем информацию о количестве подключей и значений
    DWORD subKeyCount = 0;
    DWORD valueCount = 0;
    result = RegQueryInfoKeyW(hSubKey, nullptr, nullptr, nullptr, &subKeyCount,
        nullptr, nullptr, &valueCount, nullptr, nullptr, nullptr, nullptr);

    if (result != ERROR_SUCCESS) {
        RegCloseKey(hSubKey);
        registryStats.errors++;
        registryStats.errorMessages.push_back(
            "Failed to query key info: " + std::string(subKey.begin(), subKey.end()));
        return false;
    }

    // Рекурсивно удаляем подключи
    if (subKeyCount > 0) {
        std::vector<std::wstring> subKeys;
        wchar_t keyName[MAX_PATH];
        DWORD keyNameSize = MAX_PATH;

        for (DWORD i = 0; i < subKeyCount; i++) {
            keyNameSize = MAX_PATH;
            if (RegEnumKeyExW(hSubKey, i, keyName, &keyNameSize, nullptr, nullptr, nullptr, nullptr) == ERROR_SUCCESS) {
                subKeys.push_back(keyName);
            }
        }

        for (const auto& key : subKeys) {
            std::wstring fullSubKey = subKey + L"\\" + key;
            if (dryRun) {
                Logger::getInstance().log(LogLevel::INFO, 
                    "Would delete registry key: " + std::string(fullSubKey.begin(), fullSubKey.end()));
            } else {
                if (deleteRegistryKey(hKey, fullSubKey, dryRun)) {
                    registryStats.keysDeleted++;
                }
            }
        }
    }

    // Удаляем значения
    if (valueCount > 0 && !dryRun) {
        wchar_t valueName[MAX_PATH];
        DWORD valueNameSize = MAX_PATH;

        for (DWORD i = 0; i < valueCount; i++) {
            valueNameSize = MAX_PATH;
            if (RegEnumValueW(hSubKey, i, valueName, &valueNameSize, nullptr, nullptr, nullptr, nullptr) == ERROR_SUCCESS) {
                if (deleteRegistryValue(hSubKey, subKey, valueName, dryRun)) {
                    registryStats.valuesDeleted++;
                }
            }
        }
    }

    RegCloseKey(hSubKey);
    return true;
}

bool Cleaner::deleteRegistryKey(HKEY hKey, const std::wstring& subKey, bool dryRun) {
    if (dryRun) {
        Logger::getInstance().log(LogLevel::INFO, 
            "Would delete registry key: " + std::string(subKey.begin(), subKey.end()));
        return true;
    }

    LONG result = RegDeleteTreeW(hKey, subKey.c_str());
    if (result != ERROR_SUCCESS && result != ERROR_FILE_NOT_FOUND) {
        registryStats.errors++;
        registryStats.errorMessages.push_back(
            "Failed to delete key: " + std::string(subKey.begin(), subKey.end()));
        return false;
    }
    return true;
}

bool Cleaner::deleteRegistryValue(HKEY hKey, const std::wstring& subKey, const std::wstring& valueName, bool dryRun) {
    if (dryRun) {
        Logger::getInstance().log(LogLevel::INFO, 
            "Would delete registry value: " + std::string(subKey.begin(), subKey.end()) + 
            "\\" + std::string(valueName.begin(), valueName.end()));
        return true;
    }

    LONG result = RegDeleteValueW(hKey, valueName.c_str());
    if (result != ERROR_SUCCESS && result != ERROR_FILE_NOT_FOUND) {
        registryStats.errors++;
        registryStats.errorMessages.push_back(
            "Failed to delete value: " + std::string(subKey.begin(), subKey.end()) + 
            "\\" + std::string(valueName.begin(), valueName.end()));
        return false;
    }
    return true;
}

std::vector<std::wstring> Cleaner::getObsoleteRegistryKeys() const {
    std::vector<std::wstring> keys = {
        L"Software\\Microsoft\\Windows\\CurrentVersion\\Uninstall",  // Удаленные программы
        L"Software\\Classes\\Local Settings\\Software\\Microsoft\\Windows\\Shell\\MuiCache",  // Кэш оболочки
        L"Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\RunMRU",  // История запусков
        L"Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\TypedPaths",  // История путей
        L"Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\RecentDocs"  // Недавние документы
    };
    return keys;
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

std::string Cleaner::generateBackupPath(const std::string& operationType) const {
    auto now = std::chrono::system_clock::now();
    auto time = std::chrono::system_clock::to_time_t(now);
    std::stringstream ss;
    ss << "backups/" << operationType << "_" 
       << std::put_time(std::localtime(&time), "%Y%m%d_%H%M%S");
    return ss.str();
}

bool Cleaner::createBackup(const std::string& operationType) {
    if (!isAdmin()) {
        Logger::getInstance().log(LogLevel::ERROR, 
            "Administrator privileges required for backup operations");
        return false;
    }

    BackupInfo backup;
    backup.operationType = operationType;
    backup.timestamp = std::to_string(
        std::chrono::system_clock::now().time_since_epoch().count());
    backup.backupPath = generateBackupPath(operationType);

    // Create backup directory
    std::filesystem::create_directories(backup.backupPath);

    bool success = true;
    if (operationType == "temp") {
        // Backup temporary files
        auto directories = getTempDirectories();
        for (const auto& dir : directories) {
            if (std::filesystem::exists(dir)) {
                for (const auto& entry : std::filesystem::directory_iterator(dir)) {
                    if (std::filesystem::is_regular_file(entry)) {
                        std::string targetPath = backup.backupPath + "/" + 
                            entry.path().filename().string();
                        if (backupFile(entry.path().string(), targetPath)) {
                            backup.files.push_back(entry.path().string());
                            backup.totalSize += std::filesystem::file_size(entry.path());
                        } else {
                            success = false;
                        }
                    }
                }
            }
        }
    } else if (operationType == "registry") {
        // Backup registry keys
        for (const auto& key : getObsoleteRegistryKeys()) {
            if (!backupRegistryKey(HKEY_CURRENT_USER, key, backup.backupPath)) {
                success = false;
            }
            if (!backupRegistryKey(HKEY_LOCAL_MACHINE, key, backup.backupPath)) {
                success = false;
            }
        }
    }

    if (success) {
        backupHistory.push_back(backup);
        Logger::getInstance().log(LogLevel::INFO, 
            "Backup created successfully: " + backup.backupPath);
    } else {
        Logger::getInstance().log(LogLevel::ERROR, 
            "Backup creation completed with errors");
    }

    return success;
}

bool Cleaner::backupFile(const std::string& sourcePath, const std::string& backupPath) {
    try {
        if (std::filesystem::exists(sourcePath)) {
            std::filesystem::copy_file(sourcePath, backupPath, 
                std::filesystem::copy_options::overwrite_existing);
            return true;
        }
    } catch (const std::exception& e) {
        Logger::getInstance().log(LogLevel::ERROR, 
            "Failed to backup file: " + sourcePath + " - " + e.what());
    }
    return false;
}

bool Cleaner::backupRegistryKey(HKEY hKey, const std::wstring& subKey, 
                              const std::string& backupPath) {
    HKEY hSubKey;
    LONG result = RegOpenKeyExW(hKey, subKey.c_str(), 0, KEY_READ, &hSubKey);
    
    if (result != ERROR_SUCCESS) {
        if (result != ERROR_FILE_NOT_FOUND) {
            Logger::getInstance().log(LogLevel::ERROR, 
                "Failed to open registry key for backup: " + 
                std::string(subKey.begin(), subKey.end()));
        }
        return false;
    }

    // Create backup file for this key
    std::string keyBackupPath = backupPath + "/" + 
        std::string(subKey.begin(), subKey.end()) + ".reg";
    std::ofstream backupFile(keyBackupPath);
    
    if (!backupFile.is_open()) {
        RegCloseKey(hSubKey);
        return false;
    }

    // Write key information
    backupFile << "Windows Registry Editor Version 5.00\n\n";
    backupFile << "[" << std::string(subKey.begin(), subKey.end()) << "]\n";

    // Backup values
    DWORD valueCount = 0;
    if (RegQueryInfoKeyW(hSubKey, nullptr, nullptr, nullptr, nullptr,
        nullptr, nullptr, &valueCount, nullptr, nullptr, nullptr, nullptr) == ERROR_SUCCESS) {
        
        wchar_t valueName[MAX_PATH];
        DWORD valueNameSize = MAX_PATH;
        BYTE valueData[4096];
        DWORD valueDataSize = sizeof(valueData);
        DWORD valueType;

        for (DWORD i = 0; i < valueCount; i++) {
            valueNameSize = MAX_PATH;
            valueDataSize = sizeof(valueData);
            if (RegEnumValueW(hSubKey, i, valueName, &valueNameSize, nullptr,
                &valueType, valueData, &valueDataSize) == ERROR_SUCCESS) {
                
                backupFile << "\"" << std::string(valueName, valueName + valueNameSize) << "\"=";
                
                switch (valueType) {
                    case REG_SZ:
                        backupFile << "\"" << (wchar_t*)valueData << "\"\n";
                        break;
                    case REG_DWORD:
                        backupFile << "dword:" << std::hex << *(DWORD*)valueData << "\n";
                        break;
                    // Add other value types as needed
                }
            }
        }
    }

    backupFile.close();
    RegCloseKey(hSubKey);
    return true;
}

bool Cleaner::restoreFromBackup(const std::string& backupPath) {
    if (!isAdmin()) {
        Logger::getInstance().log(LogLevel::ERROR, 
            "Administrator privileges required for restore operations");
        return false;
    }

    // Find backup info
    auto it = std::find_if(backupHistory.begin(), backupHistory.end(),
        [&backupPath](const BackupInfo& info) { return info.backupPath == backupPath; });

    if (it == backupHistory.end()) {
        Logger::getInstance().log(LogLevel::ERROR, "Backup not found: " + backupPath);
        return false;
    }

    const BackupInfo& backup = *it;
    bool success = true;

    if (backup.operationType == "temp") {
        // Restore files
        for (const auto& file : backup.files) {
            std::string sourcePath = backup.backupPath + "/" + 
                std::filesystem::path(file).filename().string();
            if (!restoreFile(sourcePath, file)) {
                success = false;
            }
        }
    } else if (backup.operationType == "registry") {
        // Restore registry keys
        for (const auto& key : backup.registryKeys) {
            // Implementation for registry restore
            // This would involve parsing the .reg file and applying changes
        }
    }

    if (success) {
        Logger::getInstance().log(LogLevel::INFO, 
            "Restore completed successfully from: " + backupPath);
    } else {
        Logger::getInstance().log(LogLevel::ERROR, 
            "Restore completed with errors");
    }

    return success;
}

bool Cleaner::restoreFile(const std::string& backupPath, const std::string& targetPath) {
    try {
        if (std::filesystem::exists(backupPath)) {
            std::filesystem::copy_file(backupPath, targetPath, 
                std::filesystem::copy_options::overwrite_existing);
            return true;
        }
    } catch (const std::exception& e) {
        Logger::getInstance().log(LogLevel::ERROR, 
            "Failed to restore file: " + targetPath + " - " + e.what());
    }
    return false;
}

std::vector<BackupInfo> Cleaner::getAvailableBackups() const {
    return backupHistory;
}

bool Cleaner::deleteBackup(const std::string& backupPath) {
    try {
        if (std::filesystem::exists(backupPath)) {
            std::filesystem::remove_all(backupPath);
            
            // Remove from history
            backupHistory.erase(
                std::remove_if(backupHistory.begin(), backupHistory.end(),
                    [&backupPath](const BackupInfo& info) { 
                        return info.backupPath == backupPath; 
                    }),
                backupHistory.end()
            );
            
            Logger::getInstance().log(LogLevel::INFO, 
                "Backup deleted successfully: " + backupPath);
            return true;
        }
    } catch (const std::exception& e) {
        Logger::getInstance().log(LogLevel::ERROR, 
            "Failed to delete backup: " + backupPath + " - " + e.what());
    }
    return false;
}

// Implementation of backup-specific cleaning functions
bool Cleaner::cleanTempFilesWithBackup(bool dryRun) {
    if (!createBackup("temp")) {
        return false;
    }
    return cleanTempFiles(dryRun);
}

bool Cleaner::cleanRegistryWithBackup(bool dryRun) {
    if (!createBackup("registry")) {
        return false;
    }
    return cleanRegistry(dryRun);
}

bool Cleaner::cleanBrowserCacheWithBackup(bool dryRun) {
    if (!createBackup("browser")) {
        return false;
    }
    return cleanBrowserCache(dryRun);
}

bool Cleaner::cleanRecycleBinWithBackup(bool dryRun) {
    // Note: Recycle bin doesn't need backup as it's already a safety mechanism
    return cleanRecycleBin(dryRun);
} 