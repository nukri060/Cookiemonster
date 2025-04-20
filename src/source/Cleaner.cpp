#include "Cleaner.h"
#include <windows.h>
#include <shlobj.h>
#include <iostream>
#include <filesystem>

Cleaner::Cleaner() : tempStats(), recycleBinStats() {}

Cleaner::~Cleaner() {}

void Cleaner::showStatistics() const {
    std::cout << "\nCleaning Statistics:\n";
    std::cout << "Temporary Files:\n";
    std::cout << "  Files deleted: " << tempStats.filesDeleted << "\n";
    std::cout << "  Errors encountered: " << tempStats.errors << "\n";
    std::cout << "Recycle Bin:\n";
    std::cout << "  Files deleted: " << recycleBinStats.filesDeleted << "\n";
    std::cout << "  Errors encountered: " << recycleBinStats.errors << "\n";
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
                            if (std::filesystem::remove(entry.path())) {
                                tempStats.filesDeleted++;
                            }
                        }
                    } catch (const std::exception& e) {
                        tempStats.errors++;
                    }
                }
            }
        } catch (const std::exception& e) {
            tempStats.errors++;
        }
    }
    return true;
}

bool Cleaner::cleanRecycleBin() {
    recycleBinStats = RecycleBinStats(); // Reset statistics
    
    try {
        // Get the number of items in recycle bin before cleaning
        SHQUERYRBINFO rbInfo = { sizeof(SHQUERYRBINFO) };
        if (SUCCEEDED(SHQueryRecycleBin(NULL, &rbInfo))) {
            HRESULT result = SHEmptyRecycleBin(NULL, NULL, SHERB_NOCONFIRMATION | SHERB_NOPROGRESSUI | SHERB_NOSOUND);
            if (SUCCEEDED(result)) {
                recycleBinStats.filesDeleted = static_cast<int>(rbInfo.i64NumItems);
                return true;
            } else {
                recycleBinStats.errors++;
                std::cerr << "Failed to empty recycle bin. Error code: " << result << std::endl;
                return false;
            }
        } else {
            recycleBinStats.errors++;
            std::cerr << "Failed to query recycle bin info." << std::endl;
            return false;
        }
    } catch (const std::exception& e) {
        recycleBinStats.errors++;
        std::cerr << "Exception while cleaning recycle bin: " << e.what() << std::endl;
        return false;
    }
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
    // TODO: Implement browser cache cleaning
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