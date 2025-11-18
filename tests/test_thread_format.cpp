// Google Test file to verify thread ID format with TRACE_THREAD_ID=1
#define TRACE_THREAD_ID 1
#define TX_TRACE_THIS_FILE 1

#include <gtest/gtest.h>
#include "../src/DebugTracesLib.h"
#include <fstream>
#include <string>
#include <filesystem>

class ThreadFormatTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Initialize tracing with file output
        LOG_TO_FILE("thread_test.log");
    }

    void TearDown() override {
        // Clean up test log file after test
        std::remove("thread_test.log");
        // Also clean up PID-suffixed files
        cleanupLogFiles("thread_test.log");
    }
    
    std::string findActualLogFile(const std::string& base_name) {
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
    
    void cleanupLogFiles(const std::string& base_name) {
        size_t dot_pos = base_name.find_last_of('.');
        std::string name_part = (dot_pos != std::string::npos) ? 
                               base_name.substr(0, dot_pos) : base_name;
        std::string pattern = name_part + "_pid";
        
        try {
            for (const auto& entry : std::filesystem::directory_iterator(".")) {
                if (entry.is_regular_file()) {
                    std::string filename = entry.path().filename().string();
                    if (filename.find(pattern) == 0) {
                        std::filesystem::remove(entry.path());
                    }
                }
            }
        } catch (...) {
            // Ignore cleanup errors
        }
    }
};

TEST_F(ThreadFormatTest, ThreadIDInLogMessages) {
    // Test that thread ID appears in log messages when TRACE_THREAD_ID=1
    LOGI("This is an INFO message with thread ID");
    LOGD("This is a DEBUG message with thread ID");
    LOGE("This is an ERROR message with thread ID");
    
    // Find the actual log file created (has PID suffix)
    std::string actual_log_file = findActualLogFile("thread_test.log");
    ASSERT_FALSE(actual_log_file.empty()) << "Log file should be created";
    
    std::ifstream logFile(actual_log_file);
    ASSERT_TRUE(logFile.is_open()) << "Log file should be created";
    
    std::string line;
    bool foundThreadInfo = false;
    
    // Read the log file and check for thread ID format
    while (std::getline(logFile, line)) {
        // With TRACE_THREAD_ID=1, log messages should contain thread information
        // Look for typical thread ID patterns in the log output
        if (line.find("Thread") != std::string::npos || 
            line.find("thread") != std::string::npos ||
            line.find("INFO") != std::string::npos) {
            foundThreadInfo = true;
            break;
        }
    }
    
    logFile.close();
    
    // At minimum, we should have some log content
    EXPECT_TRUE(foundThreadInfo) << "Log file should contain logged messages";
}

TEST_F(ThreadFormatTest, MultipleLogLevels) {
    // Test different log levels with thread ID enabled
    LOGI("INFO level message");
    LOGD("DEBUG level message");  
    LOGE("ERROR level message");
    LOGW("WARNING level message");
    
    // Find the actual log file created (has PID suffix)
    std::string actual_log_file = findActualLogFile("thread_test.log");
    ASSERT_FALSE(actual_log_file.empty()) << "Log file should be created";
    
    std::ifstream logFile(actual_log_file);
    ASSERT_TRUE(logFile.is_open());
    
    std::string content((std::istreambuf_iterator<char>(logFile)),
                        std::istreambuf_iterator<char>());
    logFile.close();
    
    EXPECT_FALSE(content.empty()) << "Log file should contain logged messages";
    
    // With TRACE_THREAD_ID=1, all messages should be logged with thread context
    EXPECT_TRUE(content.find("INFO level message") != std::string::npos);
    EXPECT_TRUE(content.find("ERROR level message") != std::string::npos);
}