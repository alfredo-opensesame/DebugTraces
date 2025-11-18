// Test file for minimal DebugTraces API - thread safety and C/C++ compatibility
#include <gtest/gtest.h>

// Test C++ interface
#include "../src/DebugTracesLib.h"

#include <fstream>
#include <string>
#include <chrono>
#include <thread>
#include <vector>
#include <atomic>
#include <filesystem>
#include <sstream>
#include <regex>

class MinimalDebugTracesTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Use a unique file name per test to avoid cross-process race conditions when
        // ctest runs multiple test cases in parallel (each test runs in its own process
        // but they previously shared the same file name). A concurrent test removing
        // the common file could lead to empty/absent content here.
        const ::testing::TestInfo* info = ::testing::UnitTest::GetInstance()->current_test_info();
        std::string uniqueName = "shared";
        if (info) {
            uniqueName = std::string(info->test_suite_name()) + "_" + info->name();
        }
        // Sanitize name (replace any spaces just in case)
        std::replace(uniqueName.begin(), uniqueName.end(), ' ', '_');
        test_file_ = std::string("test_debug_") + uniqueName + ".log";
        std::filesystem::remove(test_file_);
    }

    void TearDown() override {
        // Disable file logging and clean up
        tx_log_enable_file(false);
        tx_log_flush();
        std::filesystem::remove(test_file_);
    }

    std::string readLogFile() {
        tx_log_flush();
        
        // The logger adds PID suffix to make files process-unique
        // Find the actual file that was created
        std::string actual_file = findActualLogFile();
        if (actual_file.empty()) {
            return "";
        }
        
        std::ifstream file(actual_file);
        if (!file.is_open()) return "";
        
        std::stringstream buffer;
        buffer << file.rdbuf();
        return buffer.str();
    }
    
    std::string findActualLogFile() {
        // The logger creates files with PID suffix: basename_pidXXXX.ext
        std::string base_name = test_file_;
        size_t dot_pos = base_name.find_last_of('.');
        std::string name_part, ext_part;
        
        if (dot_pos != std::string::npos) {
            name_part = base_name.substr(0, dot_pos);
            ext_part = base_name.substr(dot_pos);
        } else {
            name_part = base_name;
            ext_part = "";
        }
        
        // Look for files matching the pattern: name_part_pidXXXX.ext
        std::string pattern = name_part + "_pid";
        
        try {
            for (const auto& entry : std::filesystem::directory_iterator(".")) {
                if (entry.is_regular_file()) {
                    std::string filename = entry.path().filename().string();
                    if (filename.find(pattern) == 0 && filename.ends_with(ext_part)) {
                        return filename;
                    }
                }
            }
        } catch (...) {
            // Fallback: try the original filename
        }
        
        return "";
    }

    std::string test_file_;
};

// Test basic macro functionality
TEST_F(MinimalDebugTracesTest, BasicMacros) {
    LOG_TO_FILE(test_file_.c_str());
    
    LOGI("Test info message: %d", 42);
    LOGW("Test warning message: %s", "warning");
    LOGE("Test error message");
    TRACE("Test trace message");
    
    std::string log_content = readLogFile();
    EXPECT_TRUE(log_content.find("Test info message: 42") != std::string::npos);
    EXPECT_TRUE(log_content.find("Test warning message: warning") != std::string::npos);
    EXPECT_TRUE(log_content.find("Test error message") != std::string::npos);
    EXPECT_TRUE(log_content.find("Test trace message") != std::string::npos);
    
    // Check format: EPOCH_MS | TID | LEVEL | file:line func() | message
    EXPECT_TRUE(log_content.find("INFO") != std::string::npos);
    EXPECT_TRUE(log_content.find("WARN") != std::string::npos);
    EXPECT_TRUE(log_content.find("ERROR") != std::string::npos);
    // DBG level has no level tag, so just check the message content from TRACE
    EXPECT_TRUE(log_content.find("Test trace message") != std::string::npos);
}

// Test RAII timer
TEST_F(MinimalDebugTracesTest, RAIITimer) {
    LOG_TO_FILE(test_file_.c_str());
    
    {
        LOGTIMER("TestOperation");
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
    
    std::string log_content = readLogFile();
    EXPECT_TRUE(log_content.find("TestOperation took") != std::string::npos);
    EXPECT_TRUE(log_content.find("ms") != std::string::npos);
}

// Test file path setting
// Helper function to find PID-suffixed log files
static std::string findPidSuffixedLogFile(const std::string& base_name) {
    size_t dot_pos = base_name.find_last_of('.');
    std::string name_part, ext_part;
    
    if (dot_pos != std::string::npos) {
        name_part = base_name.substr(0, dot_pos);
        ext_part = base_name.substr(dot_pos);
    } else {
        name_part = base_name;
        ext_part = "";
    }
    
    // Look for files matching the pattern: name_part_pidXXXX.ext
    std::string pattern = name_part + "_pid";
    
    try {
        for (const auto& entry : std::filesystem::directory_iterator(".")) {
            if (entry.is_regular_file()) {
                std::string filename = entry.path().filename().string();
                if (filename.find(pattern) == 0 && filename.ends_with(ext_part)) {
                    return filename;
                }
            }
        }
    } catch (...) {
        // Fallback: try the original filename
    }
    
    return "";
}

TEST_F(MinimalDebugTracesTest, FilePathSetting) {
    std::string custom_file = "custom_debug.log";
    std::filesystem::remove(custom_file);
    
    LOG_TO_FILE(custom_file.c_str());
    LOGI("Message in custom file");
    
    tx_log_flush();
    
    // Logger adds PID suffix, so find the actual file
    std::string actual_file = findPidSuffixedLogFile(custom_file);
    EXPECT_FALSE(actual_file.empty()) << "Custom log file should be created with PID suffix";
    
    std::ifstream file(actual_file);
    std::string content((std::istreambuf_iterator<char>(file)),
                       std::istreambuf_iterator<char>());
    EXPECT_TRUE(content.find("Message in custom file") != std::string::npos);
    
    std::filesystem::remove(custom_file);
    if (!actual_file.empty()) {
        std::filesystem::remove(actual_file);
    }
}

// Test default file name
TEST_F(MinimalDebugTracesTest, DefaultFileName) {
    std::filesystem::remove("debugtraces.log");
    
    LOG_TO_FILE();  // Should use default "debugtraces.log"
    LOGI("Default file message");
    
    tx_log_flush();
    
    // Logger adds PID suffix, so find the actual file
    std::string actual_file = findPidSuffixedLogFile("debugtraces.log");
    EXPECT_FALSE(actual_file.empty()) << "Default log file should be created with PID suffix";
    
    std::filesystem::remove("debugtraces.log");
    if (!actual_file.empty()) {
        std::filesystem::remove(actual_file);
    }
}

// Test file logging enable/disable
TEST_F(MinimalDebugTracesTest, FileLoggingControl) {
    // Initially no file logging unless LOG_TO_FILE() is called
    LOGI("This should not go to file");
    EXPECT_FALSE(std::filesystem::exists(test_file_));
    
    // Enable file logging
    LOG_TO_FILE(test_file_.c_str());
    LOGI("This should go to file");
    
    std::string log_content = readLogFile();
    EXPECT_TRUE(log_content.find("This should go to file") != std::string::npos);
    EXPECT_TRUE(log_content.find("This should not go to file") == std::string::npos);
    
    // Disable file logging
    tx_log_enable_file(false);
    LOGI("This should not go to file either");
    
    // Should not have new content
    std::string new_content = readLogFile();
    EXPECT_TRUE(new_content.find("This should not go to file either") == std::string::npos);
}

// Test thread safety with multiple threads
TEST_F(MinimalDebugTracesTest, ThreadSafety) {
    LOG_TO_FILE(test_file_.c_str());
    
    const int num_threads = 10;
    const int messages_per_thread = 100;
    std::vector<std::thread> threads;
    std::atomic<int> completed_threads{0};
    
    for (int i = 0; i < num_threads; ++i) {
        threads.emplace_back([i, &completed_threads]() {
            for (int j = 0; j < messages_per_thread; ++j) {
                LOGI("Thread %d message %d", i, j);
                LOGW("Thread %d warning %d", i, j);
                TRACE("Thread %d trace %d", i, j);
                
                // Add some timing stress
                {
                    LOGTIMER("ThreadTimer");
                    std::this_thread::sleep_for(std::chrono::microseconds(1));
                }
            }
            completed_threads++;
        });
    }
    
    // Wait for all threads
    for (auto& t : threads) {
        t.join();
    }
    
    EXPECT_EQ(completed_threads.load(), num_threads);
    
    std::string log_content = readLogFile();
    
    // Verify all threads wrote messages
    for (int i = 0; i < num_threads; ++i) {
        std::string thread_msg = "Thread " + std::to_string(i) + " message";
        EXPECT_TRUE(log_content.find(thread_msg) != std::string::npos);
    }
    
    // Count total log lines (should be num_threads * messages_per_thread * 4)
    // 4 = LOGI + LOGW + TRACE + LOGTIMER per iteration
    size_t line_count = 0;
    size_t pos = 0;
    while ((pos = log_content.find('\n', pos)) != std::string::npos) {
        line_count++;
        pos++;
    }
    
    // Should have at least the expected number of lines
    size_t expected_lines = num_threads * messages_per_thread * 4;
    EXPECT_GE(line_count, expected_lines);
}

// Test C interface compatibility
extern "C" {
    void test_c_function() {
        LOGI("Message from C function");
        LOGE("Error from C function: %d", 123);
        TRACE("C trace message");
    }
}

TEST_F(MinimalDebugTracesTest, CCompatibility) {
    LOG_TO_FILE(test_file_.c_str());
    
    test_c_function();
    
    std::string log_content = readLogFile();
    EXPECT_TRUE(log_content.find("Message from C function") != std::string::npos);
    EXPECT_TRUE(log_content.find("Error from C function: 123") != std::string::npos);
    EXPECT_TRUE(log_content.find("C trace message") != std::string::npos);
}

// Forward declarations for per-file override test functions
namespace TestDisabled {
    void disabled_function();
}

namespace TestEnabled {
    void enabled_function();
}

TEST_F(MinimalDebugTracesTest, PerFileOverride) {
    LOG_TO_FILE(test_file_.c_str());
    
    // Call functions from both namespaces
    TestDisabled::disabled_function();  // Should produce no output
    TestEnabled::enabled_function();    // Should produce output
    
    std::string log_content = readLogFile();
    
    // Only the enabled function should have logged
    EXPECT_TRUE(log_content.find("This should compile and log") != std::string::npos);
    EXPECT_TRUE(log_content.find("This should work") != std::string::npos);
    EXPECT_TRUE(log_content.find("EnabledTimer took") != std::string::npos);
    
    // The disabled function should not have logged
    EXPECT_TRUE(log_content.find("This should not compile to anything") == std::string::npos);
    EXPECT_TRUE(log_content.find("This should be a no-op") == std::string::npos);
    EXPECT_TRUE(log_content.find("DisabledTimer") == std::string::npos);
}

// Test API functions
TEST_F(MinimalDebugTracesTest, APIFunctions) {
    // Test tx_log_is_enabled
    EXPECT_TRUE(tx_log_is_enabled());
    
    // Test file path setting
    tx_log_set_file_path(test_file_.c_str());
    tx_log_enable_file(true);
    
    LOGI("API test message");
    
    std::string log_content = readLogFile();
    EXPECT_TRUE(log_content.find("API test message") != std::string::npos);
    
    // Test flush
    tx_log_flush();  // Should not crash
}

// Test format correctness 
TEST_F(MinimalDebugTracesTest, LogFormat) {
    LOG_TO_FILE(test_file_.c_str());
    
    LOGI("Format test message");
    LOGD("Debug test message");  // DBG level should not show level tag
    
    std::string log_content = readLogFile();
    
    // Check format: <DateTime> [<Level>] <filename>:<line>: <Message>
    // Should have datetime format YYYY-MM-DD HH:MM:SS.mmm
    EXPECT_TRUE(std::regex_search(log_content, std::regex(R"(\d{4}-\d{2}-\d{2} \d{2}:\d{2}:\d{2}\.\d{3})")));
    
    // Should have INFO level in brackets (no spaces)
    EXPECT_TRUE(log_content.find("[INFO]") != std::string::npos);
    
    // DEBUG level should NOT have [DEBUG] tag
    EXPECT_TRUE(log_content.find("[DEBUG]") == std::string::npos);
    EXPECT_TRUE(log_content.find("Debug test message") != std::string::npos);
    
    // Should have filename:line format
    EXPECT_TRUE(std::regex_search(log_content, std::regex(R"(dbgtracing_gtest\.cpp:\d+:)")));
    
    // Should have the message
    EXPECT_TRUE(log_content.find("Format test message") != std::string::npos);
}

// Performance test - ensure no-ops are truly no-ops
TEST_F(MinimalDebugTracesTest, PerformanceNoOps) {
    // This test verifies that disabled macros compile to no-ops
    // In practice, you'd check assembly output, but we can at least verify
    // that the disabled functions don't affect file output
    
    auto start = std::chrono::steady_clock::now();
    
    // Call disabled functions many times
    for (int i = 0; i < 10000; ++i) {
        TestDisabled::disabled_function();
    }
    
    auto end = std::chrono::steady_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
    
    // Should be very fast since it's all no-ops
    EXPECT_LT(duration.count(), 1000);  // Less than 1ms for 10k no-op calls
    
    // Should have no file created
    EXPECT_FALSE(std::filesystem::exists(test_file_));
}

// Test ANSI color formatting (we check file output since console may strip colors)
TEST_F(MinimalDebugTracesTest, ANSIColorFormat) {
    LOG_TO_FILE(test_file_.c_str());
    
    // Test different log levels
    LOGF("Fatal message");
    LOGE("Error message");
    LOGW("Warning message");
    LOGI("Info message");
    LOGD("Debug message");
    
    // Flush and read the file content (unique per test, so no cross-test interference)
    std::string log_content = readLogFile();
    
    // File output should not contain ANSI codes
    EXPECT_TRUE(log_content.find("\033[") == std::string::npos);
    
    // Should have proper level tags (except DBG)
    EXPECT_TRUE(log_content.find("[FATAL]") != std::string::npos);
    EXPECT_TRUE(log_content.find("[ERROR]") != std::string::npos);
    EXPECT_TRUE(log_content.find("[WARN]") != std::string::npos);
    EXPECT_TRUE(log_content.find("[INFO]") != std::string::npos);
    
    // DBG should NOT have [DBG] tag (no level tag at all)
    EXPECT_TRUE(log_content.find("[DBG]") == std::string::npos);
    EXPECT_TRUE(log_content.find("[DEBUG]") == std::string::npos);
    EXPECT_TRUE(log_content.find("Debug message") != std::string::npos);
}

// Test DEBUG level format (no level tag)
TEST_F(MinimalDebugTracesTest, DebugLevelFormat) {
    LOG_TO_FILE(test_file_.c_str());
    
    LOGD("This is a debug message");
    LOGI("This is an info message");
    
    std::string log_content = readLogFile();
    
    // Should have INFO with level tag
    EXPECT_TRUE(log_content.find("[INFO]") != std::string::npos);
    EXPECT_TRUE(log_content.find("This is an info message") != std::string::npos);
    
    // DEBUG should have NO level tag, just datetime and location
    EXPECT_TRUE(log_content.find("[DEBUG]") == std::string::npos);
    EXPECT_TRUE(log_content.find("This is a debug message") != std::string::npos);
    
    // Should have proper format: <DateTime> [PID:xxx] <filename>:<line>: <Message>
    EXPECT_TRUE(std::regex_search(log_content, 
        std::regex(R"(\d{4}-\d{2}-\d{2} \d{2}:\d{2}:\d{2}\.\d{3} \[PID:\d+\] dbgtracing_gtest\.cpp:\d+: This is a debug message)")));
}

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}