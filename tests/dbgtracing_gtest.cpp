
#include <gtest/gtest.h>
#include "Logger.h"
#include "DebugTracesLib.h"

#include <fstream>
#include <string>

TEST(TXLogging, FileSinkWritesAndTrims) {
  const std::string path = "txlogging_test.log";
  // Start instance and route to file
  TX_Logger::startInstance();
  auto* lg = TX_Logger::getPtr();
  lg->setLogLevel(LogLevel::LL_DEBUG);
  lg->logIntoFile(path, /*maxLines=*/50);

  // Write > 60 lines
  for (int i = 0; i < 60; ++i) {
    lg->writeLine(LogLevel::LL_INFO, __FILE__, __LINE__, "line %d", i);
  }

  // Give the background writer a moment to flush
  std::this_thread::sleep_for(std::chrono::milliseconds(200));

  // Count lines
  std::ifstream in(path);
  ASSERT_TRUE(in.is_open());
  size_t lines = 0; std::string s;
  while (std::getline(in, s)) ++lines;
  // Writer keeps 70% of max when trimming; max=50 => keep ~35..50 lines
  EXPECT_LE(lines, 50u);
  EXPECT_GE(lines, 30u);

  TX_Logger::releaseInstance();
}
