// Test file for thread ID formatting - tests both TRACE_THREAD_ID=0 and TRACE_THREAD_ID=1 scenarios
#include <gtest/gtest.h>

#define TX_TRACE_THIS_FILE 1
#include "../src/DebugTracesLib.h"

#include <fstream>
#include <string>
#include <regex>
#include <filesystem>

class ThreadIDFormatTest : public ::testing::Test {
protected:
    void SetUp() override {
        test_file_ = "thread_test_debug.log";
        // Clean up any existing file
        std::remove(test_file_.c_str());
    }

    void TearDown() override {
        tx_log_enable_file(false);
        std::remove(test_file_.c_str());
        // Also clean up PID-suffixed files
        cleanupLogFiles();
    }
    
    void cleanupLogFiles() {
        size_t dot_pos = test_file_.find_last_of('.');
        std::string name_part = (dot_pos != std::string::npos) ? 
                               test_file_.substr(0, dot_pos) : test_file_;
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

    std::string readLogFile() {
        // Find the actual log file created (has PID suffix)
        std::string actual_file = findActualLogFile();
        if (actual_file.empty()) {
            return "";
        }
        
        std::ifstream file(actual_file);
        if (!file.is_open()) {
            return "";
        }
        
        std::stringstream buffer;
        buffer << file.rdbuf();
        file.close();
        return buffer.str();
    }
    
    std::string findActualLogFile() {
        size_t dot_pos = test_file_.find_last_of('.');
        std::string name_part, ext_part;
        
        if (dot_pos != std::string::npos) {
            name_part = test_file_.substr(0, dot_pos);
            ext_part = test_file_.substr(dot_pos);
        } else {
            name_part = test_file_;
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

// Test thread ID format based on TRACE_THREAD_ID compile-time setting
TEST_F(ThreadIDFormatTest, ThreadIDFormat) {
    LOG_TO_FILE(test_file_.c_str());
    
    LOGI("Info message for format test");
    LOGD("Debug message for format test");
    LOGE("Error message for format test");
    
    std::string log_content = readLogFile();
    
    // Should always have datetime format
    EXPECT_TRUE(std::regex_search(log_content, std::regex(R"(\d{4}-\d{2}-\d{2} \d{2}:\d{2}:\d{2}\.\d{3})")));
    
    // DBG level should never show [DBG] tag
    EXPECT_TRUE(log_content.find("[DBG]") == std::string::npos);
    
#if TRACE_THREAD_ID
    // With TRACE_THREAD_ID=1: Format should be [<Level>][<ThreadID>] for INFO/ERROR
    EXPECT_TRUE(std::regex_search(log_content, std::regex(R"(\[INFO\]\[[0-9a-fA-F]+\])")));
    EXPECT_TRUE(std::regex_search(log_content, std::regex(R"(\[ERROR\]\[[0-9a-fA-F]+\])")));
    
    // DBG should have thread ID but no level tag: [<ThreadID>] <filename>:<line>: <Message>
    EXPECT_TRUE(std::regex_search(log_content, std::regex(R"(\d{4}-\d{2}-\d{2} \d{2}:\d{2}:\d{2}\.\d{3} \[[0-9a-fA-F]+\] .*Debug message for format test)")));
#else
    // With TRACE_THREAD_ID=0: Format should be [<Level>][PID:xxx] for INFO/ERROR
    EXPECT_TRUE(std::regex_search(log_content, std::regex(R"(\[INFO\]\[PID:\d+\])")));
    EXPECT_TRUE(std::regex_search(log_content, std::regex(R"(\[ERROR\]\[PID:\d+\])")));
    
    // DBG should have no level tag but should have PID: [PID:xxx] <filename>:<line>: <Message>
    EXPECT_TRUE(std::regex_search(log_content, std::regex(R"(\d{4}-\d{2}-\d{2} \d{2}:\d{2}:\d{2}\.\d{3} \[PID:\d+\] [^[]*Debug message for format test)")));
    
    // Should NOT contain thread ID patterns
    EXPECT_FALSE(std::regex_search(log_content, std::regex(R"(\[[0-9a-fA-F]+\])")));
#endif
    
    // Should have all messages
    EXPECT_TRUE(log_content.find("Info message for format test") != std::string::npos);
    EXPECT_TRUE(log_content.find("Debug message for format test") != std::string::npos);
    EXPECT_TRUE(log_content.find("Error message for format test") != std::string::npos);
}

// Test thread ID consistency (only applicable when TRACE_THREAD_ID=1)
TEST_F(ThreadIDFormatTest, ThreadIDConsistency) {
#if TRACE_THREAD_ID
    LOG_TO_FILE(test_file_.c_str());
    
    LOGI("First message");
    LOGI("Second message");
    
    std::string log_content = readLogFile();
    
    // Extract thread IDs from both messages
    std::regex thread_id_regex(R"(\[INFO\]\[([0-9a-fA-F]+)\])");
    std::sregex_iterator iter(log_content.begin(), log_content.end(), thread_id_regex);
    std::sregex_iterator end;
    
    EXPECT_NE(iter, end);  // Should find at least one match
    std::string first_thread_id = (*iter)[1].str();
    
    ++iter;
    EXPECT_NE(iter, end);  // Should find second match
    std::string second_thread_id = (*iter)[1].str();
    
    // Thread IDs should be the same for messages from the same thread
    EXPECT_EQ(first_thread_id, second_thread_id);
#else
    // When TRACE_THREAD_ID=0, this test is not applicable - just verify basic functionality
    LOG_TO_FILE(test_file_.c_str());
    
    LOGI("Thread consistency not tested when TRACE_THREAD_ID=0");
    
    std::string log_content = readLogFile();
    EXPECT_TRUE(log_content.find("Thread consistency not tested when TRACE_THREAD_ID=0") != std::string::npos);
#endif
}

// Test runtime enable/disable of thread ID tracing
TEST_F(ThreadIDFormatTest, RuntimeThreadIDControl) {
    LOG_TO_FILE(test_file_.c_str());
    
    // Test enabling thread ID tracing at runtime
    ENABLE_THREAD_ID_TRACING();
    LOGI("Message with thread ID enabled");
    
    // Test disabling thread ID tracing at runtime  
    DISABLE_THREAD_ID_TRACING();
    LOGI("Message with thread ID disabled");
    
    // Re-enable to test toggle
    ENABLE_THREAD_ID_TRACING();
    LOGI("Message with thread ID re-enabled");
    
    std::string log_content = readLogFile();
    
    // Verify the messages exist in the log
    EXPECT_TRUE(log_content.find("Message with thread ID enabled") != std::string::npos);
    EXPECT_TRUE(log_content.find("Message with thread ID disabled") != std::string::npos);
    EXPECT_TRUE(log_content.find("Message with thread ID re-enabled") != std::string::npos);
    
    // Note: The actual thread ID presence/absence testing depends on the compile-time TRACE_THREAD_ID setting
    // This test verifies the macros work without runtime errors
}