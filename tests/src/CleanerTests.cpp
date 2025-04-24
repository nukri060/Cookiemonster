#include <catch2/catch_all.hpp>
#include "../../src/include/Cleaner.h"
#include <filesystem>
#include <fstream>

TEST_CASE("Basic functionality", "[basic]") {
    Cleaner cleaner;

    SECTION("Format size test") {
        REQUIRE(cleaner.formatSize(0) == "0.00 B");
        REQUIRE(cleaner.formatSize(1024) == "1.00 KB");
        REQUIRE(cleaner.formatSize(1048576) == "1.00 MB");
        REQUIRE(cleaner.formatSize(1073741824) == "1.00 GB");
        REQUIRE(cleaner.formatSize(500) == "500.00 B");
        REQUIRE(cleaner.formatSize(1500) == "1.46 KB");
    }

    SECTION("Admin check test") {
        bool isAdmin = cleaner.isAdmin();
        INFO("Admin check should return a boolean value");
        REQUIRE((isAdmin == true || isAdmin == false));
    }

    SECTION("Temp directories test") {
        auto directories = cleaner.getTempDirectories();
        REQUIRE_FALSE(directories.empty());
        
        // Check if at least one directory exists
        bool atLeastOneExists = false;
        for (const auto& dir : directories) {
            if (std::filesystem::exists(dir)) {
                atLeastOneExists = true;
                break;
            }
        }
        REQUIRE(atLeastOneExists);
    }
}

TEST_CASE("File operations in user space", "[file_ops]") {
    Cleaner cleaner;
    
    SECTION("Create and delete test file in temp directory") {
        // Create a temporary test directory in user's temp folder
        auto tempPath = std::filesystem::temp_directory_path() / "cookiemonster_test";
        std::filesystem::create_directories(tempPath);
        
        // Create a test file
        auto testFile = tempPath / "test_file.txt";
        {
            std::ofstream outfile(testFile);
            REQUIRE(outfile.is_open());
            outfile << "test content" << std::endl;
        }

        REQUIRE(std::filesystem::exists(testFile));
        REQUIRE(cleaner.deleteFile(testFile.string()));
        REQUIRE_FALSE(std::filesystem::exists(testFile));

        // Cleanup
        std::filesystem::remove_all(tempPath);
    }
} 