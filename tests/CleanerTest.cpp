#include <gtest/gtest.h>
#include "../src/include/Cleaner.h"
#include <filesystem>
#include <fstream>
#include <chrono>

class CleanerTest : public ::testing::Test {
protected:
    Cleaner* cleaner;
    std::wstring testDir;

    void SetUp() override {
        cleaner = new Cleaner();
        // Create test directory
        testDir = L"test_temp_dir";
        std::filesystem::create_directory(testDir);
    }

    void TearDown() override {
        // Clean up test directory
        std::filesystem::remove_all(testDir);
        delete cleaner;
    }

    void createTestFile(const std::wstring& filename, size_t size = 1024) {
        std::ofstream file(filename, std::ios::binary);
        std::vector<char> data(size, 'A');
        file.write(data.data(), data.size());
    }
};

// Test basic file operations
TEST_F(CleanerTest, FileValidation) {
    std::wstring testFile = testDir + L"\\test.txt";
    createTestFile(testFile);

    // Test file validation
    EXPECT_TRUE(cleaner->validateFile(testFile));
    EXPECT_FALSE(cleaner->validateFile(L"nonexistent.txt"));
}

// Test directory validation
TEST_F(CleanerTest, DirectoryValidation) {
    EXPECT_TRUE(cleaner->validateDirectory(testDir));
    EXPECT_FALSE(cleaner->validateDirectory(L"nonexistent_dir"));
}

// Test path safety checks
TEST_F(CleanerTest, PathSafety) {
    // Test absolute path
    EXPECT_TRUE(cleaner->isPathSafe(testDir));
    
    // Test relative path
    EXPECT_FALSE(cleaner->isPathSafe(L"relative/path"));
    
    // Test excluded path
    std::vector<std::wstring> excluded = {L"excluded"};
    cleaner->setExcludedPaths(excluded);
    EXPECT_FALSE(cleaner->isPathSafe(L"C:\\excluded\\test.txt"));
}

// Test file type validation
TEST_F(CleanerTest, FileTypeValidation) {
    std::wstring testFile = testDir + L"\\test.exe";
    createTestFile(testFile);

    // Test excluded file type
    EXPECT_FALSE(cleaner->isFileTypeAllowed(testFile));
    
    // Test allowed file type
    std::wstring allowedFile = testDir + L"\\test.txt";
    createTestFile(allowedFile);
    EXPECT_TRUE(cleaner->isFileTypeAllowed(allowedFile));
}

// Test scan cache functionality
TEST_F(CleanerTest, ScanCache) {
    cleaner->setScanCacheEnabled(true);
    
    std::vector<std::wstring> files;
    uint64_t totalSize;
    
    // First scan should populate cache
    EXPECT_FALSE(cleaner->getCachedScan(testDir, files, totalSize));
    
    // Simulate scan
    std::vector<std::wstring> testFiles = {testDir + L"\\test1.txt", testDir + L"\\test2.txt"};
    for (const auto& file : testFiles) {
        createTestFile(file);
    }
    cleaner->updateScanCache(testDir, testFiles, 2048);
    
    // Second scan should use cache
    EXPECT_TRUE(cleaner->getCachedScan(testDir, files, totalSize));
    EXPECT_EQ(files.size(), testFiles.size());
    EXPECT_EQ(totalSize, 2048);
}

// Test batch processing
TEST_F(CleanerTest, BatchProcessing) {
    // Create test files
    std::vector<std::wstring> testFiles;
    for (int i = 0; i < 5; ++i) {
        std::wstring filename = testDir + L"\\test" + std::to_wstring(i) + L".txt";
        createTestFile(filename);
        testFiles.push_back(filename);
    }
    
    // Test batch splitting
    auto batches = cleaner->splitIntoBatches(testFiles);
    EXPECT_GT(batches.size(), 0);
    
    // Test batch processing
    EXPECT_TRUE(cleaner->processFileBatch(batches[0], true)); // Dry run
}

// Test security settings
TEST_F(CleanerTest, SecuritySettings) {
    // Test max scan depth
    cleaner->setMaxScanDepth(5);
    EXPECT_EQ(cleaner->getMaxScanDepth(), 5);
    
    // Test symlink following
    cleaner->setFollowSymlinks(true);
    EXPECT_TRUE(cleaner->isFollowingSymlinks());
    
    // Test excluded file types
    std::vector<std::string> excludedTypes = {".exe", ".dll"};
    cleaner->setExcludedFileTypes(excludedTypes);
    auto types = cleaner->getExcludedFileTypes();
    EXPECT_EQ(types.size(), excludedTypes.size());
}

// Test performance settings
TEST_F(CleanerTest, PerformanceSettings) {
    // Test scan cache
    cleaner->setScanCacheEnabled(false);
    EXPECT_FALSE(cleaner->isScanCacheEnabled());
    
    // Test max threads
    cleaner->setMaxThreads(8);
    EXPECT_EQ(cleaner->getMaxThreads(), 8);
    
    // Test batch size
    cleaner->setBatchSize(50);
    EXPECT_EQ(cleaner->getBatchSize(), 50);
}

// Test error handling
TEST_F(CleanerTest, ErrorHandling) {
    // Test invalid settings
    cleaner->setMaxScanDepth(-1);
    EXPECT_GT(cleaner->getMaxScanDepth(), 0);
    
    cleaner->setMaxThreads(0);
    EXPECT_GT(cleaner->getMaxThreads(), 0);
    
    cleaner->setBatchSize(-1);
    EXPECT_GT(cleaner->getBatchSize(), 0);
}

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
} 