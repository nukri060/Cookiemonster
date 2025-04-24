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

struct BrowserCacheStats {
    int filesDeleted = 0;
    int errors = 0;
    uint64_t bytesFreed = 0;  // Size in bytes
    std::string browserName;
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

    // Browser-specific cleaning functions
    bool cleanChromiumCache();  // For Chrome and Edge
    bool cleanFirefoxCache();

    // Utility functions
    bool isAdmin() const;
    void showStatistics() const;
    std::string formatSize(uint64_t bytes) const;  // Helper to format size
    std::vector<std::wstring> getTempDirectories() const;
    bool deleteFile(const std::string& path);

private:
    // Helper methods
    bool deleteDirectory(const std::wstring& path);
    std::wstring getLocalAppData() const;
    std::vector<std::wstring> getBrowserPaths() const;
    
    TempFilesStats tempStats;
    RecycleBinStats recycleBinStats;
    std::vector<BrowserCacheStats> browserStats;
}; 