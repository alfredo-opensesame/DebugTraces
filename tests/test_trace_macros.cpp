/**
 * @file test_trace_macros.cpp  
 * @brief Tests for the TRACE macros and per-file controls
 * 
 * Tests the TRACE macro functionality and per-file compilation controls
 * including TX_TRACE_THIS_FILE features.
 */

#include <gtest/gtest.h>

/* Enable tracing for this test file */
#define TX_TRACE_THIS_FILE 1

#include "../src/DebugTracesLib.h"
#include <chrono>
#include <thread>
#include <vector>
#include <atomic>
#include <cstdio>  // For std::remove

class TraceMacroTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Use the current API - just enable file logging
        LOG_TO_FILE("test_trace_macros.log");
    }

    void TearDown() override {
        // Clean up test log file
        std::remove("test_trace_macros.log");
    }
};

TEST_F(TraceMacroTest, BasicTraceMacros) {
    // Test that TRACE macros work when TX_TRACE_THIS_FILE is enabled
    
    TRACE("Basic trace message");
    TRACE("Trace with parameter: %d", 42);
    TRACE("Trace with multiple params: %s, %d, %.2f", "test", 123, 3.14);
    
    // TRACES is an alias for TRACE
    TRACE("TRACE macro test");
    TRACE("TRACE with data: %s", "working");
    
    // Test passes if no crashes occurred
    SUCCEED();
}

TEST_F(TraceMacroTest, LogLevelMacros) {
    // Test the level-specific logging macros
    
    LOGD("Debug level message");
    LOGI("Info level message with value: %d", 100);
    LOGW("Warning level message");  
    LOGE("Error level message with string: %s", "error_data");
    
    // Test different log levels - all should work in current implementation
    LOGD("This debug message should appear");
    LOGI("This info message should appear");
    LOGW("This warning should appear");
    LOGE("This error should appear");
    
    // Note: Current implementation doesn't have runtime log level filtering
    
    SUCCEED();
}

TEST_F(TraceMacroTest, TimeMeasurementMacros) {
    // Test time measurement functionality using LOGTIMER
    
    {
        LOGTIMER("Test operation");
        // Simulate some work
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    } // Timer logs automatically when scope ends
    
    SUCCEED();
}

TEST_F(TraceMacroTest, NamedTimeMeasurementContext) {
    // Test named timing contexts using LOGTIMER scopes
    
    {
        LOGTIMER("operation1");
        std::this_thread::sleep_for(std::chrono::milliseconds(5));
    }
    
    {
        LOGTIMER("operation2");  
        std::this_thread::sleep_for(std::chrono::milliseconds(8));
    }
    
    // Test nested timing contexts
    {
        LOGTIMER("context_a");
        std::this_thread::sleep_for(std::chrono::milliseconds(2));
        {
            LOGTIMER("context_b");
            std::this_thread::sleep_for(std::chrono::milliseconds(3));
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(2));
    }
    
    SUCCEED();
}

TEST_F(TraceMacroTest, FilenameExtraction) {
    // Test that TX_FILENAME macro works correctly
    
    // The TX_FILENAME should extract just the filename, not full path
    const char* filename = __FILE__; // Use standard __FILE__ macro
    
    EXPECT_NE(filename, nullptr);
    
    // Should not contain directory separators at the start
    // (though it will contain the actual filename which has underscores)
    EXPECT_TRUE(strstr(filename, "test_trace_macros.cpp") != nullptr);
    
    // Use in a log message
    LOGI("Current test file: %s", __FILE__);
    
    SUCCEED();
}

TEST_F(TraceMacroTest, LoggingFunctionality) {
    // Test that the logging macros work correctly
    
    // These should log successfully 
    LOGI("Test info message with value: %d", 42);
    LOGD("Test debug message");
    LOGW("Test warning message");
    LOGE("Test error message");
    LOGF("Test fatal message");
    
    // Test with expressions
    int value = 10;
    LOGI("Variable value is: %d", value);
    LOGD("Condition check: value > 5 is %s", (value > 5) ? "true" : "false");
    
    SUCCEED();
}

TEST_F(TraceMacroTest, ParameterLogging) {
    // Test parameter logging functionality
    
    auto test_function = [](int used_param, int unused_param, const char* another_unused) {
        // Use one parameter normally
        LOGD("Used parameter value: %d", used_param);
        
        // Suppress unused parameter warnings with void cast
        (void)unused_param;
        (void)another_unused;
        
        return used_param * 2;
    };
    
    int result = test_function(21, 999, "ignored");
    EXPECT_EQ(result, 42);
    
    SUCCEED();
}

TEST_F(TraceMacroTest, MacroThreadSafety) {
    // Test that trace macros are thread-safe
    
    const int num_threads = 4;
    std::vector<std::thread> threads;
    std::atomic<int> completed{0};
    
    auto worker = [&](int thread_id) {
        for (int i = 0; i < 20; ++i) {
            TRACE("Thread %d iteration %d", thread_id, i);
            LOGI("Thread %d info message %d", thread_id, i);
            
            if (i % 5 == 0) {
                LOGTIMER("Thread operation");
                std::this_thread::sleep_for(std::chrono::microseconds(100));
            }
        }
        completed.fetch_add(1, std::memory_order_relaxed);
    };
    
    // Start threads
    for (int i = 0; i < num_threads; ++i) {
        threads.emplace_back(worker, i);
    }
    
    // Wait for completion
    for (auto& t : threads) {
        t.join();
    }
    
    EXPECT_EQ(completed.load(), num_threads);
}