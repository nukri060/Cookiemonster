#include <iostream>
#include "Cleaner.h"

int main() {
    std::cout << "CookieMonster v0.1" << std::endl;
    
    Cleaner cleaner;
    
    if (!cleaner.isAdmin()) {
        std::cout << "Warning: Running without administrator privileges. Some features may not work properly." << std::endl;
    }
    
    // Test basic functionality
    cleaner.cleanTempFiles();
    cleaner.cleanRecycleBin();
    
    // Show cleaning statistics
    cleaner.showStatistics();
    
    return 0;
} 