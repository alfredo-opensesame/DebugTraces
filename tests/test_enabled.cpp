// Separate file with logging enabled for testing per-file override
#define TX_TRACE_THIS_FILE 1
#include "../src/DebugTracesLib.h"

namespace TestEnabled {
    void enabled_function() {
        LOGI("This should compile and log");
        TRACE("This should work");
        LOGTIMER("EnabledTimer");
    }
}