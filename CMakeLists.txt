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
option(DBGTR_CLANG_TIDY  "Enable Clang-Tidy static analysis" OFF)
option(DBGTR_ASAN        "Enable AddressSanitizer"       OFF)
option(DBGTR_TSAN        "Enable ThreadSanitizer"        OFF)
option(DBGTR_UBSAN       "Enable UndefinedBehaviorSanitizer" OFF)

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

# ----- Static Analysis & Sanitizers -----

# Clang-Tidy setup
if (DBGTR_CLANG_TIDY)
  find_program(CLANG_TIDY_EXE NAMES "clang-tidy" PATHS "/opt/homebrew/opt/llvm/bin" NO_DEFAULT_PATH)
  if (NOT CLANG_TIDY_EXE)
    find_program(CLANG_TIDY_EXE NAMES "clang-tidy")
  endif()
  
  if (CLANG_TIDY_EXE)
    set_target_properties(${DBGTR_TARGET_NAME} PROPERTIES
      CXX_CLANG_TIDY "${CLANG_TIDY_EXE};--config-file=${CMAKE_CURRENT_SOURCE_DIR}/cmake/.clang-tidy;-header-filter=.*"
    )
    message(STATUS "Clang-Tidy enabled: ${CLANG_TIDY_EXE}")
  else()
    message(WARNING "Clang-Tidy requested but not found")
  endif()
endif()

# Sanitizer support (mutually exclusive - choose one)
set(SANITIZER_COMPILE_FLAGS "")
set(SANITIZER_LINK_FLAGS "")

if (DBGTR_ASAN)
  if (DBGTR_TSAN OR DBGTR_UBSAN)
    message(FATAL_ERROR "Cannot enable multiple sanitizers simultaneously")
  endif()
  set(SANITIZER_COMPILE_FLAGS "-fsanitize=address;-fno-omit-frame-pointer;-g")
  set(SANITIZER_LINK_FLAGS "-fsanitize=address")
  message(STATUS "AddressSanitizer enabled")
elseif (DBGTR_TSAN)
  if (DBGTR_ASAN OR DBGTR_UBSAN)
    message(FATAL_ERROR "Cannot enable multiple sanitizers simultaneously")
  endif()
  set(SANITIZER_COMPILE_FLAGS "-fsanitize=thread;-fno-omit-frame-pointer;-g")
  set(SANITIZER_LINK_FLAGS "-fsanitize=thread")
  message(STATUS "ThreadSanitizer enabled")
elseif (DBGTR_UBSAN)
  if (DBGTR_ASAN OR DBGTR_TSAN)
    message(FATAL_ERROR "Cannot enable multiple sanitizers simultaneously")
  endif()
  set(SANITIZER_COMPILE_FLAGS "-fsanitize=undefined;-fno-omit-frame-pointer;-g")
  set(SANITIZER_LINK_FLAGS "-fsanitize=undefined")
  message(STATUS "UndefinedBehaviorSanitizer enabled")
endif()

# Apply sanitizer flags to main library
if (SANITIZER_COMPILE_FLAGS AND NOT MSVC)
  target_compile_options(${DBGTR_TARGET_NAME} PRIVATE ${SANITIZER_COMPILE_FLAGS})
  target_link_options(${DBGTR_TARGET_NAME} PRIVATE ${SANITIZER_LINK_FLAGS})
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
    
    # Apply sanitizers to tests
    if (SANITIZER_COMPILE_FLAGS AND NOT MSVC)
      target_compile_options(${DBGTR_TARGET_NAME}_tests PRIVATE ${SANITIZER_COMPILE_FLAGS})
      target_link_options(${DBGTR_TARGET_NAME}_tests PRIVATE ${SANITIZER_LINK_FLAGS})
    endif()
    
    # Apply Clang-Tidy to tests
    if (DBGTR_CLANG_TIDY AND CLANG_TIDY_EXE)
      set_target_properties(${DBGTR_TARGET_NAME}_tests PROPERTIES
        CXX_CLANG_TIDY "${CLANG_TIDY_EXE};--config-file=${CMAKE_CURRENT_SOURCE_DIR}/cmake/.clang-tidy;-header-filter=.*"
      )
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

# ----- Custom Targets for Static Analysis & Sanitizers -----

# Clang-Tidy target (run manually)
find_program(MANUAL_CLANG_TIDY_EXE NAMES "clang-tidy" PATHS "/opt/homebrew/opt/llvm/bin" NO_DEFAULT_PATH)
if (NOT MANUAL_CLANG_TIDY_EXE)
  find_program(MANUAL_CLANG_TIDY_EXE NAMES "clang-tidy")
endif()

if (MANUAL_CLANG_TIDY_EXE)
  add_custom_target(clang-tidy
    COMMAND ${MANUAL_CLANG_TIDY_EXE}
      --config-file=${CMAKE_CURRENT_SOURCE_DIR}/cmake/.clang-tidy
      -header-filter=.*
      ${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp
      ${CMAKE_CURRENT_SOURCE_DIR}/src/*.h
      --
      -std=c++20
      -I${CMAKE_CURRENT_SOURCE_DIR}/src
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    COMMENT "Running Clang-Tidy static analysis"
    VERBATIM
  )
endif()

# ASAN test target
add_custom_target(test-asan
  COMMAND ${CMAKE_COMMAND} -DDBGTR_ASAN=ON -DCMAKE_BUILD_TYPE=Debug -S ${CMAKE_SOURCE_DIR} -B ${CMAKE_BINARY_DIR}/asan
  COMMAND ${CMAKE_COMMAND} --build ${CMAKE_BINARY_DIR}/asan
  COMMAND ${CMAKE_CTEST_COMMAND} --test-dir ${CMAKE_BINARY_DIR}/asan --output-on-failure
  COMMENT "Building and running tests with AddressSanitizer"
  VERBATIM
)

# TSAN test target
add_custom_target(test-tsan
  COMMAND ${CMAKE_COMMAND} -DDBGTR_TSAN=ON -DCMAKE_BUILD_TYPE=Debug -S ${CMAKE_SOURCE_DIR} -B ${CMAKE_BINARY_DIR}/tsan
  COMMAND ${CMAKE_COMMAND} --build ${CMAKE_BINARY_DIR}/tsan
  COMMAND ${CMAKE_CTEST_COMMAND} --test-dir ${CMAKE_BINARY_DIR}/tsan --output-on-failure
  COMMENT "Building and running tests with ThreadSanitizer"
  VERBATIM
)

# UBSAN test target
add_custom_target(test-ubsan
  COMMAND ${CMAKE_COMMAND} -DDBGTR_UBSAN=ON -DCMAKE_BUILD_TYPE=Debug -S ${CMAKE_SOURCE_DIR} -B ${CMAKE_BINARY_DIR}/ubsan
  COMMAND ${CMAKE_COMMAND} --build ${CMAKE_BINARY_DIR}/ubsan
  COMMAND ${CMAKE_CTEST_COMMAND} --test-dir ${CMAKE_BINARY_DIR}/ubsan --output-on-failure
  COMMENT "Building and running tests with UndefinedBehaviorSanitizer"
  VERBATIM
)

# Combined static analysis target
add_custom_target(analyze
  DEPENDS clang-tidy
  COMMENT "Running all static analysis tools"
)

# All sanitizers target (runs them sequentially)
add_custom_target(test-all-sanitizers
  DEPENDS test-asan test-tsan test-ubsan
  COMMENT "Running tests with all sanitizers"
)
