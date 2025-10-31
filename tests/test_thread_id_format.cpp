// Test file for thread ID formatting - tests both TRACE_THREAD_ID=0 and TRACE_THREAD_ID=1 scenarios
#include <gtest/gtest.h>

#define TX_TRACE_THIS_FILE 1
#include "../src/DebugTracesLib.h"

#include <fstream>
#include <string>
#include <regex>

class ThreadIDFormatTest : public ::testing::Test {
protected:
    void SetUp() override {
        test_file_ = "thread_test_debug.log";
        // Clean up any existing file
        std::remove(test_file_.c_str());
    }

    void TearDown() override {
        std::remove(test_file_.c_str());
    }

    std::string readLogFile() {
        std::ifstream file(test_file_);
        if (!file.is_open()) {
            return "";
        }
        
        std::stringstream buffer;
        buffer << file.rdbuf();
        file.close();
        return buffer.str();
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
    // With TRACE_THREAD_ID=0: Format should be [<Level>] for INFO/ERROR
    EXPECT_TRUE(std::regex_search(log_content, std::regex(R"(\[INFO\] )")));
    EXPECT_TRUE(std::regex_search(log_content, std::regex(R"(\[ERROR\] )")));
    
    // DBG should have no level tag or thread ID: <filename>:<line>: <Message>
    EXPECT_TRUE(std::regex_search(log_content, std::regex(R"(\d{4}-\d{2}-\d{2} \d{2}:\d{2}:\d{2}\.\d{3} [^[]*Debug message for format test)")));
    
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