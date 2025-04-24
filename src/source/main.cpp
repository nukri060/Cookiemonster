#include "Cleaner.h"
#include <iostream>

int main() {
    Cleaner cleaner;
    std::cout << "CookieMonster - Windows System Cleanup Utility\n" << std::endl;

    if (!cleaner.isAdmin()) {
        std::cout << "Warning: Running without administrator privileges.\n";
        std::cout << "Some operations may be restricted.\n" << std::endl;
    }

    // Clean temporary files
    std::cout << "Cleaning temporary files..." << std::endl;
    if (cleaner.cleanTempFiles()) {
        std::cout << "Temporary files cleaned successfully." << std::endl;
    }

    // Clean browser cache (requires admin)
    if (cleaner.isAdmin()) {
        std::cout << "Cleaning browser cache..." << std::endl;
        if (cleaner.cleanBrowserCache()) {
            std::cout << "Browser cache cleaned successfully." << std::endl;
        }
    }

    // Clean recycle bin (requires admin)
    if (cleaner.isAdmin()) {
        std::cout << "Cleaning recycle bin..." << std::endl;
        if (cleaner.cleanRecycleBin()) {
            std::cout << "Recycle bin cleaned successfully." << std::endl;
        }
    }

    // Clean registry (requires admin)
    if (cleaner.isAdmin()) {
        std::cout << "Cleaning registry..." << std::endl;
        if (cleaner.cleanRegistry()) {
            std::cout << "Registry cleaned successfully." << std::endl;
        }
    }

    // Show statistics
    cleaner.showStatistics();

    return 0;
} 