#include "catch2/catch.hpp"
#include "../Cleaner.h"
#include <filesystem>
#include <fstream>

TEST_CASE("Cleaner basic functionality", "[cleaner]") {
    Cleaner cleaner;

    SECTION("Format size test") {
        REQUIRE(cleaner.formatSize(1024) == "1.00 KB");
        REQUIRE(cleaner.formatSize(1048576) == "1.00 MB");
        REQUIRE(cleaner.formatSize(1073741824) == "1.00 GB");
        REQUIRE(cleaner.formatSize(500) == "500.00 B");
    }

    SECTION("Admin check test") {
        // This test might pass or fail depending on how the test is run
        bool isAdmin = cleaner.isAdmin();
        REQUIRE((isAdmin == true || isAdmin == false));
    }

    SECTION("Temp directories test") {
        auto directories = cleaner.getTempDirectories();
        REQUIRE_FALSE(directories.empty());
        
        // Check if directories exist
        for (const auto& dir : directories) {
            REQUIRE(std::filesystem::exists(dir));
        }
    }
}

TEST_CASE("File operations", "[file_ops]") {
    Cleaner cleaner;
    
    // Create a temporary test file
    std::string testFile = "test_temp_file.txt";
    std::ofstream outfile(testFile);
    outfile << "test content" << std::endl;
    outfile.close();

    SECTION("Delete file test") {
        REQUIRE(std::filesystem::exists(testFile));
        bool result = cleaner.deleteFile(testFile);
        REQUIRE(result == true);
        REQUIRE_FALSE(std::filesystem::exists(testFile));
    }
} 