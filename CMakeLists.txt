cmake_minimum_required(VERSION 3.20)

project(DebugTracing VERSION 1.0.0 LANGUAGES CXX)

set(DBGTR_TARGET_NAME      dbgtracing)
set(DBGTR_TARGET_NAMESPACE DBGTX::)
set(DBGTR_TARGET_ALIAS     "${DBGTR_TARGET_NAMESPACE}${DBGTR_TARGET_NAME}")

# Options
option(DBGTR_BUILD_TESTS "Build Debug Tracing tests"     ON)
option(DBGTR_INSTALL     "Install Debug Tracing targets" ON)
option(DBGTR_WARNINGS    "Enable extra warnings"         ON)
option(DBGTR_WERROR      "Treat warnings as errors"      OFF)

# If DBGTX::dbgtracing already exists (e.g., provided upstream), skip local definition
if (TARGET ${DBGTR_TARGET_ALIAS})
  message(STATUS "${DBGTR_TARGET_ALIAS} already exists; skipping local ${DBGTR_TARGET_NAME} at ${CMAKE_CURRENT_SOURCE_DIR}")
  return()
endif()

# --- Library target (static, as requested) ---
if (NOT TARGET ${DBGTR_TARGET_NAME})
  add_library(${DBGTR_TARGET_NAME} STATIC
      src/Logger.cpp
      src/Writer.cpp
      src/DebugTracesLib.h
      src/Logger.h
      src/Writer.h
  )
endif()

# Prefer PIC for static libs that may be linked into shared libs
set_target_properties(${DBGTR_TARGET_NAME} PROPERTIES POSITION_INDEPENDENT_CODE ON)

# Public headers during build & install
# NOTE: If you later move public headers to include/dbgtracing, change BUILD_INTERFACE accordingly.
target_include_directories(${DBGTR_TARGET_NAME}
  PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src>
    $<INSTALL_INTERFACE:include/${DBGTR_TARGET_NAME}>
)

# C++ standard + warnings
target_compile_features(${DBGTR_TARGET_NAME} PUBLIC cxx_std_20)

if (DBGTR_WARNINGS)
  if (MSVC)
    target_compile_options(${DBGTR_TARGET_NAME} PRIVATE /W4 /permissive-)
    if (DBGTR_WERROR)
      target_compile_options(${DBGTR_TARGET_NAME} PRIVATE /WX)
    endif()
    target_compile_definitions(${DBGTR_TARGET_NAME} PRIVATE _CRT_SECURE_NO_WARNINGS WIN32_LEAN_AND_MEAN NOMINMAX)
  else()
    target_compile_options(${DBGTR_TARGET_NAME} PRIVATE -Wall -Wextra -Wpedantic)
    if (DBGTR_WERROR)
      target_compile_options(${DBGTR_TARGET_NAME} PRIVATE -Werror)
    endif()
  endif()
endif()

# Namespaced alias for consumers
add_library(${DBGTR_TARGET_ALIAS} ALIAS ${DBGTR_TARGET_NAME})

# ----- Installation (optional) -----
include(GNUInstallDirs)

if (DBGTR_INSTALL)
  # Install headers
  install(FILES
      src/DebugTracesLib.h
      src/Logger.h
      src/Writer.h
    DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}/${DBGTR_TARGET_NAME}")

  # Install library and export
  install(TARGETS ${DBGTR_TARGET_NAME}
    EXPORT "DebugTracingTargets"
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    INCLUDES DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}/${DBGTR_TARGET_NAME}"
  )

  # Generate and install a Config file for find_package()
  include(CMakePackageConfigHelpers)
  write_basic_package_version_file(
    "${CMAKE_CURRENT_BINARY_DIR}/DebugTracingConfigVersion.cmake"
    VERSION ${PROJECT_VERSION}
    COMPATIBILITY SameMajorVersion
  )

  configure_package_config_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/cmake/DebugTracingConfig.cmake.in"
    "${CMAKE_CURRENT_BINARY_DIR}/DebugTracingConfig.cmake"
    INSTALL_DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/${PROJECT_NAME}"
  )

  install(EXPORT "DebugTracingTargets"
    NAMESPACE ${DBGTR_TARGET_NAMESPACE}
    DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/${PROJECT_NAME}")

  install(FILES
    "${CMAKE_CURRENT_BINARY_DIR}/DebugTracingConfig.cmake"
    "${CMAKE_CURRENT_BINARY_DIR}/DebugTracingConfigVersion.cmake"
    DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/${PROJECT_NAME}")
endif()

# ----- Tests -----
if (DBGTR_BUILD_TESTS)
  include(CTest)
  enable_testing()

  # Try config package first, then module
  find_package(GTest CONFIG QUIET)
  if (NOT GTest_FOUND)
    find_package(GTest QUIET)
  endif()

  if (GTest_FOUND)
    # Automatically discover all test files in tests directory
    file(GLOB DBGTR_TEST_SOURCES "tests/*.cpp")
    
    # Comprehensive DebugTraces test suite - single target with all test files
    add_executable(${DBGTR_TARGET_NAME}_tests ${DBGTR_TEST_SOURCES})
    
    target_compile_features(${DBGTR_TARGET_NAME}_tests PRIVATE cxx_std_17)
    target_link_libraries(${DBGTR_TARGET_NAME}_tests PRIVATE
      ${DBGTR_TARGET_ALIAS}
      GTest::gtest
      GTest::gtest_main
    )
    
    # Enable GTEST macro for resetWriter function
    target_compile_definitions(${DBGTR_TARGET_NAME}_tests PRIVATE GTEST=1)
    
    # Thread support is essential for our thread safety tests
    find_package(Threads REQUIRED)
    target_link_libraries(${DBGTR_TARGET_NAME}_tests PRIVATE Threads::Threads)
    
    # Platform-specific libraries
    if(UNIX AND NOT APPLE)
      # Linux may need rt for clock_gettime in timing macros
      target_link_libraries(${DBGTR_TARGET_NAME}_tests PRIVATE rt)
    endif()
    
    # Compiler-specific flags for robust testing
    if(MSVC)
      target_compile_options(${DBGTR_TARGET_NAME}_tests PRIVATE /W4)
      target_compile_definitions(${DBGTR_TARGET_NAME}_tests PRIVATE _CRT_SECURE_NO_WARNINGS)
    else()
      target_compile_options(${DBGTR_TARGET_NAME}_tests PRIVATE -Wall -Wextra -pthread)
    endif()
    
    # Register tests with CTest
    include(GoogleTest OPTIONAL RESULT_VARIABLE HAVE_GTEST_MODULE)
    if (HAVE_GTEST_MODULE)
      gtest_discover_tests(${DBGTR_TARGET_NAME}_tests
        PROPERTIES
          TIMEOUT 60
          LABELS "DebugTraces;UnitTests;Comprehensive"
      )
    else()
      add_test(NAME ${DBGTR_TARGET_NAME}_tests COMMAND ${DBGTR_TARGET_NAME}_tests)
      set_tests_properties(${DBGTR_TARGET_NAME}_tests PROPERTIES
        TIMEOUT 60
        LABELS "DebugTraces;UnitTests;Comprehensive"
      )
    endif()
    
    message(STATUS "DebugTraces comprehensive test suite enabled with all tests in single target")
  else()
    message(STATUS "GTest not found; ${DBGTR_TARGET_NAME} tests will not be built")
  endif()
endif()
