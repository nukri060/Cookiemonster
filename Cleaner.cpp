#include "Cleaner.h"
#include <windows.h>
#include <shlobj.h>
#include <iostream>
#include <filesystem>

Cleaner::Cleaner() {
    std::cout << "Cleaner initialized" << std::endl;
}

Cleaner::~Cleaner() {
    std::cout << "Cleaner destroyed" << std::endl;
}

bool Cleaner::cleanTempFiles() {
    std::cout << "Starting temp files cleanup..." << std::endl;
    std::vector<std::wstring> tempDirs = getTempDirectories();
    bool success = true;

    for (const auto& dir : tempDirs) {
        try {
            for (const auto& entry : std::filesystem::directory_iterator(dir)) {
                try {
                    if (std::filesystem::is_regular_file(entry)) {
                        std::filesystem::remove(entry.path());
                    }
                }
                catch (const std::exception& e) {
                    std::wcerr << L"Error deleting file: " << entry.path().wstring() << std::endl;
                    success = false;
                }
            }
        }
        catch (const std::exception& e) {
            std::wcerr << L"Error accessing directory: " << dir << std::endl;
            success = false;
        }
    }

    return success;
}

bool Cleaner::cleanBrowserCache() {
    // TODO: Implement browser cache cleaning
    return false;
}

bool Cleaner::cleanRecycleBin() {
    std::cout << "Starting recycle bin cleanup..." << std::endl;
    SHEmptyRecycleBinW(NULL, NULL, SHERB_NOCONFIRMATION | SHERB_NOPROGRESSUI | SHERB_NOSOUND);
    return true;
}

bool Cleaner::cleanRegistry() {
    // TODO: Implement registry cleaning
    return false;
}

bool Cleaner::isAdmin() const {
    BOOL isAdmin = FALSE;
    PSID adminGroup = NULL;
    SID_IDENTIFIER_AUTHORITY NtAuthority = SECURITY_NT_AUTHORITY;

    if (AllocateAndInitializeSid(&NtAuthority, 2,
        SECURITY_BUILTIN_DOMAIN_RID,
        DOMAIN_ALIAS_RID_ADMINS,
        0, 0, 0, 0, 0, 0,
        &adminGroup)) {
        if (!CheckTokenMembership(NULL, adminGroup, &isAdmin)) {
            isAdmin = FALSE;
        }
        FreeSid(adminGroup);
    }

    return isAdmin == TRUE;
}

void Cleaner::showStatistics() const {
    // TODO: Implement statistics display
}

std::vector<std::wstring> Cleaner::getTempDirectories() const {
    std::vector<std::wstring> tempDirs;
    
    // Get Windows temp directory
    wchar_t tempPath[MAX_PATH];
    GetTempPathW(MAX_PATH, tempPath);
    tempDirs.push_back(tempPath);

    // Get user temp directory
    wchar_t userTempPath[MAX_PATH];
    if (SUCCEEDED(SHGetFolderPathW(NULL, CSIDL_LOCAL_APPDATA, NULL, 0, userTempPath))) {
        std::wstring userTemp(userTempPath);
        userTemp += L"\\Temp";
        tempDirs.push_back(userTemp);
    }

    return tempDirs;
}

bool Cleaner::deleteFile(const std::string& path) {
    try {
        if (std::filesystem::exists(path)) {
            std::filesystem::remove(path);
            return true;
        }
    }
    catch (const std::exception& e) {
        std::cerr << "Error deleting file: " << e.what() << std::endl;
    }
    return false;
} 