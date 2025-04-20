#pragma once
#include <string>
#include <vector>

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

private:
    // Helper methods
    bool deleteFile(const std::string& path);
    bool deleteDirectory(const std::wstring& path);
    std::vector<std::wstring> getTempDirectories() const;
    
    // Statistics
    struct TempFilesStats {
        int filesDeleted = 0;
        int errors = 0;
    } tempStats;
}; 