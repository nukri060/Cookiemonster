#include <iostream>
#include <windows.h>
#include "Cleaner.h"

void showMenu() {
    std::cout << "\n=== CookieMonster v1.0 ===\n";
    std::cout << "1. Clean Temporary Files\n";
    std::cout << "2. Clean Recycle Bin\n";
    std::cout << "3. Clean All\n";
    std::cout << "4. Exit\n";
    std::cout << "Select an option: ";
}

int main() {
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);

    // Check for administrator rights
    BOOL isAdmin = FALSE;
    SID_IDENTIFIER_AUTHORITY NtAuthority = SECURITY_NT_AUTHORITY;
    PSID AdministratorsGroup;
    
    if (AllocateAndInitializeSid(&NtAuthority, 2, SECURITY_BUILTIN_DOMAIN_RID, DOMAIN_ALIAS_RID_ADMINS, 0, 0, 0, 0, 0, 0, &AdministratorsGroup)) {
        if (!CheckTokenMembership(NULL, AdministratorsGroup, &isAdmin)) {
            isAdmin = FALSE;
        }
        FreeSid(AdministratorsGroup);
    }

    if (!isAdmin) {
        std::cout << "Warning: Program is running without administrator rights!\n";
        std::cout << "Some features may not work correctly.\n";
        std::cout << "Please run the program as administrator.\n\n";
    }

    Cleaner cleaner;
    int choice;

    do {
        showMenu();
        std::cin >> choice;

        switch (choice) {
            case 1:
                std::cout << "\nCleaning temporary files...\n";
                cleaner.cleanTempFiles();
                cleaner.showStatistics();
                break;
            case 2:
                std::cout << "\nCleaning recycle bin...\n";
                cleaner.cleanRecycleBin();
                cleaner.showStatistics();
                break;
            case 3:
                std::cout << "\nPerforming full cleanup...\n";
                cleaner.cleanTempFiles();
                cleaner.cleanRecycleBin();
                cleaner.showStatistics();
                break;
            case 4:
                std::cout << "\nExiting program...\n";
                break;
            default:
                std::cout << "\nInvalid choice. Please try again.\n";
                break;
        }
    } while (choice != 4);

    return 0;
} 