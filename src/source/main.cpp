#include "Cleaner.h"
#include <iostream>
#include <string>
#include <vector>
#include <algorithm>

void printHelp() {
    std::cout << "CookieMonster - Windows System Cleanup Utility\n\n"
              << "Usage: cookiemonster [options]\n\n"
              << "Options:\n"
              << "  --help, -h           Show this help message\n"
              << "  --dry-run, -d        Perform a dry run without deleting files\n"
              << "  --exclude=PATH       Exclude specific paths (can be used multiple times)\n"
              << "  --include=PATH       Include only specific paths (can be used multiple times)\n"
              << "  --no-log             Disable console logging\n"
              << "  --temp               Clean temporary files\n"
              << "  --browser            Clean browser cache\n"
              << "  --recycle            Clean recycle bin\n"
              << "  --registry           Clean registry\n"
              << "  --all                Clean all (default if no specific options provided)\n";
}

int main(int argc, char* argv[]) {
    Cleaner cleaner;
    bool dryRun = false;
    bool cleanTemp = false;
    bool cleanBrowser = false;
    bool cleanRecycle = false;
    bool cleanRegistry = false;
    bool showHelp = false;
    bool noLog = false;
    std::vector<std::wstring> excludedPaths;
    std::vector<std::wstring> includedPaths;

    // Parse command line arguments
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        
        if (arg == "--help" || arg == "-h") {
            showHelp = true;
        } else if (arg == "--dry-run" || arg == "-d") {
            dryRun = true;
        } else if (arg == "--no-log") {
            noLog = true;
        } else if (arg == "--temp") {
            cleanTemp = true;
        } else if (arg == "--browser") {
            cleanBrowser = true;
        } else if (arg == "--recycle") {
            cleanRecycle = true;
        } else if (arg == "--registry") {
            cleanRegistry = true;
        } else if (arg == "--all") {
            cleanTemp = cleanBrowser = cleanRecycle = cleanRegistry = true;
        } else if (arg.find("--exclude=") == 0) {
            std::string path = arg.substr(10);
            excludedPaths.push_back(std::wstring(path.begin(), path.end()));
        } else if (arg.find("--include=") == 0) {
            std::string path = arg.substr(10);
            includedPaths.push_back(std::wstring(path.begin(), path.end()));
        }
    }

    // If no specific options are provided, clean all
    if (!cleanTemp && !cleanBrowser && !cleanRecycle && !cleanRegistry) {
        cleanTemp = cleanBrowser = cleanRecycle = cleanRegistry = true;
    }

    if (showHelp) {
        printHelp();
        return 0;
    }

    // Configure logger
    Logger::getInstance().setConsoleOutput(!noLog);
    Logger::getInstance().log(LogLevel::INFO, "CookieMonster started" + std::string(dryRun ? " (dry run)" : ""));

    // Set excluded and included paths
    if (!excludedPaths.empty()) {
        cleaner.setExcludedPaths(excludedPaths);
    }
    if (!includedPaths.empty()) {
        cleaner.setIncludedPaths(includedPaths);
    }

    // Check for admin privileges
    if (!cleaner.isAdmin()) {
        Logger::getInstance().log(LogLevel::WARNING, 
            "Running without administrator privileges. Some operations may be restricted.");
    }

    // Perform cleaning operations
    if (cleanTemp) {
        Logger::getInstance().log(LogLevel::INFO, "Cleaning temporary files...");
        if (cleaner.cleanTempFiles(dryRun)) {
            Logger::getInstance().log(LogLevel::INFO, "Temporary files cleaned successfully.");
        }
    }

    if (cleanBrowser && cleaner.isAdmin()) {
        Logger::getInstance().log(LogLevel::INFO, "Cleaning browser cache...");
        if (cleaner.cleanBrowserCache(dryRun)) {
            Logger::getInstance().log(LogLevel::INFO, "Browser cache cleaned successfully.");
        }
    }

    if (cleanRecycle && cleaner.isAdmin()) {
        Logger::getInstance().log(LogLevel::INFO, "Cleaning recycle bin...");
        if (cleaner.cleanRecycleBin(dryRun)) {
            Logger::getInstance().log(LogLevel::INFO, "Recycle bin cleaned successfully.");
        }
    }

    if (cleanRegistry && cleaner.isAdmin()) {
        Logger::getInstance().log(LogLevel::INFO, "Cleaning registry...");
        if (cleaner.cleanRegistry(dryRun)) {
            Logger::getInstance().log(LogLevel::INFO, "Registry cleaned successfully.");
        }
    }

    // Show statistics
    cleaner.showStatistics();

    Logger::getInstance().log(LogLevel::INFO, "CookieMonster completed");
    return 0;
} 