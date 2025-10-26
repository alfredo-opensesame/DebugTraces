cmake_minimum_required(VERSION 3.20)

project(DebugTracing VERSION 1.0.0 LANGUAGES CXX)

set(DBGTR_TARGET_NAME      dbgtracing)
set(DBGTR_TARGET_NAMESPACE DBGTX::)
set(DBGTR_TARGET_ALIAS     "${DBGTR_TARGET_NAMESPACE}${DBGTR_TARGET_NAME}")

# Options
option(DBGTR_BUILD_TESTS "Build Debug Tracing tests"     OFF)
option(DBGTR_INSTALL     "Install Debug Tracing targets" ON)
option(DBGTR_WARNINGS    "Enable extra warnings"         ON)

# If DBGTX::dbgtracing already exists (e.g., provided by another subdir or an installed package),
# skip defining a local copy to avoid duplicate-target errors.
if (TARGET ${DBGTR_TARGET_ALIAS})
  message(STATUS "${DBGTR_TARGET_ALIAS} already exists; skipping local DebugTracesLib at ${CMAKE_CURRENT_SOURCE_DIR}")
  return()
endif()

# Library target (create only if not already defined)
if (NOT TARGET ${DBGTR_TARGET_NAME})
  add_library(${DBGTR_TARGET_NAME} STATIC
      src/Logger.cpp
      src/Writer.cpp
      src/DebugTracesLib.h
      src/Logger.h
      src/Writer.h
  )
endif()

# Public headers live in this directory during the build; install under include/dbgtracing
target_include_directories(${DBGTR_TARGET_NAME}
  PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src>
    $<INSTALL_INTERFACE:"include/${DBGTR_TARGET_NAME}">  # Install under include/dbgtracing
)

# C++ standard + warnings
target_compile_features(${DBGTR_TARGET_NAME} PUBLIC cxx_std_20)

if (DBGTR_WARNINGS)
  if (MSVC)
    target_compile_options(${DBGTR_TARGET_NAME} PRIVATE /W4 /permissive-)
    target_compile_definitions(${DBGTR_TARGET_NAME} PRIVATE _CRT_SECURE_NO_WARNINGS WIN32_LEAN_AND_MEAN NOMINMAX)
  else()
    target_compile_options(${DBGTR_TARGET_NAME} PRIVATE -Wall -Wextra -Wpedantic)
  endif()
endif()

# Namespaced alias for consumers
add_library(${DBGTR_TARGET_ALIAS} ALIAS ${DBGTR_TARGET_NAME})

# ----- Installation (optional but recommended) -----
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
    EXPORT "${PROJECT_NAME}Targets"
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
  )

  # Generate and install a Config file for find_package()
  include(CMakePackageConfigHelpers)
  write_basic_package_version_file(
    "${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}ConfigVersion.cmake"
    VERSION ${PROJECT_VERSION}
    COMPATIBILITY SameMajorVersion
  )

  configure_package_config_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/cmake/${PROJECT_NAME}Config.cmake.in"
    "${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}Config.cmake"
    INSTALL_DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/${PROJECT_NAME}"
  )

  install(EXPORT "${PROJECT_NAME}Targets"
    NAMESPACE ${DBGTR_TARGET_NAMESPACE}
    DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/${PROJECT_NAME}")

  install(FILES
    "${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}Config.cmake"
    "${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}ConfigVersion.cmake"
    DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/${PROJECT_NAME}")
endif()


if (TXLOG_BUILD_TESTS)
  include(CTest)
  enable_testing()
  # Expect GTest to be available in the parent project (or provide your own FetchContent here)
  find_package(GTest QUIET)
  if (GTest_FOUND)
    add_executable(${DBGTR_TARGET_NAME}_tests "tests/${DBGTR_TARGET_NAME}_gtest.cpp")
    target_link_libraries(${DBGTR_TARGET_NAME}_tests PRIVATE ${DBGTR_TARGET_ALIAS} GTest::gtest GTest::gtest_main GTest::gmock)
  target_compile_features(${DBGTR_TARGET_NAME}_tests PRIVATE cxx_std_20)
    add_test(NAME ${DBGTR_TARGET_NAME}_tests COMMAND ${DBGTR_TARGET_NAME}_tests)
  else()
    message(STATUS "GTest not found; ${DBGTR_TARGET_NAME}_tests will not be built")
  endif()
endif()
