#include <iostream>
#include "Cleaner.h"

int main() {
    std::cout << "CookieMonster v0.1" << std::endl;
    std::cout << "Initial development started..." << std::endl;
    
    Cleaner cleaner;
    
    // Test basic functionality
    cleaner.cleanTempFiles();
    cleaner.cleanRecycleBin();
    
    return 0;
} 