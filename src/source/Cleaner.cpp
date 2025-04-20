#include "Cleaner.h"
#include <windows.h>
#include <shlobj.h>
#include <iostream>
#include <filesystem>
#include <iomanip>
#include <sstream>

Cleaner::Cleaner() : tempStats(), recycleBinStats() {}

Cleaner::~Cleaner() {}

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
    std::cout << "\nCleaning Statistics:\n";
    std::cout << "Temporary Files:\n";
    std::cout << "  Files deleted: " << tempStats.filesDeleted << "\n";
    std::cout << "  Space freed: " << formatSize(tempStats.bytesFreed) << "\n";
    std::cout << "  Errors encountered: " << tempStats.errors << "\n";
    std::cout << "Recycle Bin:\n";
    std::cout << "  Files deleted: " << recycleBinStats.filesDeleted << "\n";
    std::cout << "  Space freed: " << formatSize(recycleBinStats.bytesFreed) << "\n";
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
                            uint64_t fileSize = std::filesystem::file_size(entry.path());
                            if (std::filesystem::remove(entry.path())) {
                                tempStats.filesDeleted++;
                                tempStats.bytesFreed += fileSize;
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
    recycleBinStats = RecycleBinStats();
    
    try {
        // Load Shell32.dll
        HMODULE hShell32 = LoadLibraryW(L"Shell32.dll");
        if (!hShell32) {
            std::cerr << "Error loading Shell32.dll: " << GetLastError() << std::endl;
            recycleBinStats.errors++;
            return false;
        }

        // Get function pointers
        typedef HRESULT (WINAPI *SHQueryRecycleBinW_t)(LPCWSTR, LPSHQUERYRBINFO);
        typedef HRESULT (WINAPI *SHEmptyRecycleBinW_t)(HWND, LPCWSTR, DWORD);
        
        auto SHQueryRecycleBinW = (SHQueryRecycleBinW_t)GetProcAddress(hShell32, "SHQueryRecycleBinW");
        auto SHEmptyRecycleBinW = (SHEmptyRecycleBinW_t)GetProcAddress(hShell32, "SHEmptyRecycleBinW");

        if (!SHQueryRecycleBinW || !SHEmptyRecycleBinW) {
            std::cerr << "Failed to get function pointers" << std::endl;
            FreeLibrary(hShell32);
            recycleBinStats.errors++;
            return false;
        }

        // Get recycle bin info
        SHQUERYRBINFO rbInfo;
        ZeroMemory(&rbInfo, sizeof(SHQUERYRBINFO));
        rbInfo.cbSize = sizeof(SHQUERYRBINFO);

        // Query recycle bin info for all drives
        HRESULT hr = SHQueryRecycleBinW(NULL, &rbInfo);
        if (SUCCEEDED(hr)) {
            std::cout << "Found " << rbInfo.i64NumItems << " items in recycle bin ("
                      << formatSize(rbInfo.i64Size) << ")" << std::endl;

            // Empty the recycle bin
            hr = SHEmptyRecycleBinW(NULL, NULL, SHERB_NOCONFIRMATION | SHERB_NOPROGRESSUI | SHERB_NOSOUND);
            if (SUCCEEDED(hr)) {
                recycleBinStats.filesDeleted = static_cast<int>(rbInfo.i64NumItems);
                recycleBinStats.bytesFreed = rbInfo.i64Size;
                std::cout << "Successfully emptied recycle bin" << std::endl;
                FreeLibrary(hShell32);
                return true;
            } else {
                std::cerr << "Failed to empty recycle bin. Error code: 0x" << std::hex << hr << std::dec << std::endl;
                recycleBinStats.errors++;
                FreeLibrary(hShell32);
                return false;
            }
        } else {
            std::cerr << "Failed to query recycle bin info. Error code: 0x" << std::hex << hr << std::dec << std::endl;
            recycleBinStats.errors++;
            FreeLibrary(hShell32);
            return false;
        }
    } catch (const std::exception& e) {
        std::cerr << "Exception while cleaning recycle bin: " << e.what() << std::endl;
        recycleBinStats.errors++;
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