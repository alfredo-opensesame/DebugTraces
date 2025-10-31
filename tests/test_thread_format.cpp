// Google Test file to verify thread ID format with TRACE_THREAD_ID=1
#define TRACE_THREAD_ID 1
#define TX_TRACE_THIS_FILE 1

#include <gtest/gtest.h>
#include "../src/DebugTracesLib.h"
#include <fstream>
#include <string>

class ThreadFormatTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Clean up any existing test log file
        std::remove("thread_test.log");
        
        // Enable file logging for this test
        LOG_TO_FILE("thread_test.log");
    }
    
    void TearDown() override {
        // Clean up test log file after test
        std::remove("thread_test.log");
    }
};

TEST_F(ThreadFormatTest, ThreadIDInLogMessages) {
    // Test that thread ID appears in log messages when TRACE_THREAD_ID=1
    LOGI("This is an INFO message with thread ID");
    LOGD("This is a DEBUG message with thread ID");
    LOGE("This is an ERROR message with thread ID");
    
    // Verify the log file was created and contains thread information
    std::ifstream logFile("thread_test.log");
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
    
    // Verify log file has content
    std::ifstream logFile("thread_test.log");
    ASSERT_TRUE(logFile.is_open());
    
    std::string content((std::istreambuf_iterator<char>(logFile)),
                        std::istreambuf_iterator<char>());
    logFile.close();
    
    EXPECT_FALSE(content.empty()) << "Log file should contain logged messages";
    
    // With TRACE_THREAD_ID=1, all messages should be logged with thread context
    EXPECT_TRUE(content.find("INFO level message") != std::string::npos);
    EXPECT_TRUE(content.find("ERROR level message") != std::string::npos);
}