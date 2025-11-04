#pragma once

/**
 * @file TraceColors.h
 * @brief ANSI color code definitions for debug tracing and test output
 * 
 * This file provides standardized ANSI color codes for consistent formatting
 * across different types of debug traces and test outputs.
 * 
 * Compatible with both C and C++ projects.
 */

#ifdef __cplusplus
extern "C" {
#endif

#define ANSI_API "\033[1;92m"  // Bright bold green for API traces
// =============================================================================
// ANSI Reset Code
// =============================================================================
#define ANSI_RESET "\033[0m"

// =============================================================================
// Background Colors for Message Flow Tracing
// =============================================================================
#define ANSI_CLIENT_TO_MASTER "\033[42;30m"  // Light green background, black text
#define ANSI_MASTER_TO_CLIENT "\033[43;30m"  // Light yellow background, black text
#define ANSI_CALLBACK         "\033[45;30m"  // Light magenta background, black text

// =============================================================================
// Background Colors for Constructor/Destructor and Allocation Tracing
// =============================================================================
#define ANSI_LIGHT_BLUE_BG   "\033[104m"         // Light blue background for allocations
#define ANSI_LIGHT_ORANGE_BG "\033[48;5;214m"    // Light orange background for deallocations

// =============================================================================
// Text Colors for Thread and General Tracing
// =============================================================================
#define ANSI_BRIGHT_BLUE  "\033[94m"             // Bright blue text for threads
#define ANSI_BRIGHT_GREEN "\033[92m"             // Bright green text for success/requests
#define ANSI_BRIGHT_RED   "\033[91m"             // Bright red text for errors/shutdown
#define ANSI_BRIGHT_CYAN  "\033[96m"             // Bright cyan text for encoders/controls
#define ANSI_BRIGHT_MAGENTA "\033[95m"           // Bright magenta text for displays/timecode
#define ANSI_BRIGHT_YELLOW "\033[93m"            // Bright yellow text for state/jogwheel
#define ANSI_BRIGHT_WHITE "\033[97m"             // Bright white text for buttons/touch

#ifdef __cplusplus
}
#endif