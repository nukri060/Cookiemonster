#include "Cleaner.h"
#include <windows.h>
#include <shlobj.h>
#include <iostream>
#include <filesystem>
#include <sstream>
#include <iomanip>

Cleaner::Cleaner() : tempStats() {}

Cleaner::~Cleaner() {}

void Cleaner::showStatistics() const {
    std::cout << "\nCleaning Statistics:\n";
    std::cout << "Files deleted: " << tempStats.filesDeleted << "\n";
    std::cout << "Space freed: " << formatSize(tempStats.bytesFreed) << "\n";
    std::cout << "Errors encountered: " << tempStats.errors << "\n";
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

bool Cleaner::cleanTempFiles() {
    tempStats = TempFilesStats();
    auto directories = getTempDirectories();
    
    for (const auto& dir : directories) {
        try {
            if (std::filesystem::exists(dir)) {
                for (const auto& entry : std::filesystem::directory_iterator(dir)) {
                    try {
                        if (std::filesystem::is_regular_file(entry)) {
                            uintmax_t fileSize = std::filesystem::file_size(entry.path());
                            if (std::filesystem::remove(entry.path())) {
                                tempStats.filesDeleted++;
                                tempStats.bytesFreed += fileSize;
                            }
                        }
                    } catch (const std::exception& e) {
                        tempStats.errors++;
                        std::wcerr << L"Error processing file: " << entry.path().wstring() << std::endl;
                    }
                }
            }
        } catch (const std::exception& e) {
            tempStats.errors++;
            std::wcerr << L"Error accessing directory: " << dir << std::endl;
        }
    }
    return true;
}

bool Cleaner::cleanRecycleBin() {
    SHEmptyRecycleBin(NULL, NULL, SHERB_NOCONFIRMATION | SHERB_NOPROGRESSUI | SHERB_NOSOUND);
    return true;
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
    if (!isAdmin()) {
        std::cerr << "Administrator privileges required for browser cache cleaning." << std::endl;
        return false;
    }

    std::vector<std::wstring> browserPaths = {
        L"\\Google\\Chrome\\User Data\\Default\\Cache",
        L"\\Microsoft\\Edge\\User Data\\Default\\Cache",
        L"\\Mozilla\\Firefox\\Profiles"
    };

    wchar_t localAppDataPath[MAX_PATH];
    if (!SUCCEEDED(SHGetFolderPathW(NULL, CSIDL_LOCAL_APPDATA, NULL, 0, localAppDataPath))) {
        std::cerr << "Failed to get Local AppData path." << std::endl;
        return false;
    }

    std::wstring basePath = std::wstring(localAppDataPath);
    bool success = true;

    for (const auto& path : browserPaths) {
        std::wstring fullPath = basePath + path;
        try {
            if (std::filesystem::exists(fullPath)) {
                if (path.find(L"Firefox") != std::wstring::npos) {
                    // Firefox has multiple profile folders
                    for (const auto& entry : std::filesystem::directory_iterator(fullPath)) {
                        if (entry.is_directory()) {
                            std::wstring cachePath = entry.path().wstring() + L"\\cache2";
                            if (std::filesystem::exists(cachePath)) {
                                std::filesystem::remove_all(cachePath);
                            }
                        }
                    }
                } else {
                    // Chrome and Edge
                    if (std::filesystem::exists(fullPath)) {
                        std::filesystem::remove_all(fullPath);
                    }
                }
            }
        } catch (const std::exception& e) {
            std::wcerr << L"Error cleaning browser cache: " << fullPath << std::endl;
            success = false;
        }
    }

    return success;
}

bool Cleaner::cleanRegistry() {
    if (!isAdmin()) {
        std::cerr << "Administrator privileges required for registry cleaning." << std::endl;
        return false;
    }

    HKEY hKey;
    bool success = true;

    // Clean temporary registry entries
    const wchar_t* regPaths[] = {
        L"Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\RecentDocs",
        L"Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\RunMRU",
        L"Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\TypedPaths"
    };

    for (const auto& path : regPaths) {
        if (RegOpenKeyExW(HKEY_CURRENT_USER, path, 0, KEY_ALL_ACCESS, &hKey) == ERROR_SUCCESS) {
            if (RegDeleteKeyW(hKey, L"") == ERROR_SUCCESS) {
                RegCloseKey(hKey);
                // Recreate the key to maintain structure
                RegCreateKeyExW(HKEY_CURRENT_USER, path, 0, NULL, 0, KEY_ALL_ACCESS, NULL, &hKey, NULL);
            } else {
                std::wcerr << L"Failed to clean registry path: " << path << std::endl;
                success = false;
            }
            RegCloseKey(hKey);
        }
    }

    // Clean temporary Internet Explorer cache registry entries
    if (RegOpenKeyExW(HKEY_CURRENT_USER, L"Software\\Microsoft\\Windows\\CurrentVersion\\Internet Settings\\Cache", 0, KEY_ALL_ACCESS, &hKey) == ERROR_SUCCESS) {
        DWORD value = 0;
        if (RegSetValueExW(hKey, L"Persistent", 0, REG_DWORD, (BYTE*)&value, sizeof(value)) != ERROR_SUCCESS) {
            success = false;
        }
        RegCloseKey(hKey);
    }

    return success;
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