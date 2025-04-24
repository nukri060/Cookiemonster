#pragma once
#include <string>
#include <vector>
#include <iostream>
#include <windows.h>
#include <shlobj.h>

struct TempFilesStats {
    int filesDeleted = 0;
    int errors = 0;
    uint64_t bytesFreed = 0;  // Size in bytes
};

struct RecycleBinStats {
    int filesDeleted = 0;
    int errors = 0;
    uint64_t bytesFreed = 0;  // Size in bytes
};

class Cleaner {
public:
    Cleaner();
    ~Cleaner();

    // Core cleaning functions
    bool cleanTempFiles();
    bool cleanBrowserCache();
    bool cleanRecycleBin();
    bool cleanRegistry();

    // Utility functions
    bool isAdmin() const;
    void showStatistics() const;
    std::string formatSize(uint64_t bytes) const;  // Helper to format size

private:
    // Helper methods
    bool deleteFile(const std::string& path);
    bool deleteDirectory(const std::wstring& path);
    std::vector<std::wstring> getTempDirectories() const;
    
    TempFilesStats tempStats;
    RecycleBinStats recycleBinStats;
}; 