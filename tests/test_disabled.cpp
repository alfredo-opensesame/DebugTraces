// Separate file with logging disabled for testing per-file override
#define TX_TRACE_THIS_FILE 0
#include "../src/DebugTracesLib.h"

namespace TestDisabled {
    void disabled_function() {
        LOGI("This should not compile to anything");
        TRACE("This should be a no-op");
        LOGTIMER("DisabledTimer");
    }
}