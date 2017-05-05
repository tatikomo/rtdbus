#include "glog/logging.h"
#include "gtest/gtest.h"

#include <unistd.h>
#include <stdint.h>
#include <stdlib.h>

#include "tool_metrics.hpp"

using namespace tool;

Metrics *metric_server = NULL;

TEST(TestMETRICS, CREATE)
{
  metric_server = new Metrics();
  ASSERT_TRUE(metric_server != NULL);
}

TEST(TestMETRICS, PROCESS)
{
  uint64_t sum = 0;

  std::cout << "clock ticks num per second: " << sysconf(_SC_CLK_TCK) << std::endl;

  for (int iter = 0; iter < 14; iter++)
  {
    metric_server->before();

    for (int32_t i=0; i<((rand() % 100000) + 100000000); i++)
    {
//      usleep((rand() % 50) + 1);
      sum += i;
    }

    std::cout << sum << std::endl;

    metric_server->after();

    metric_server->statistic();

    std::cout << std::endl;
  }
}

TEST(TestMETRICS, DESTROY)
{
  delete metric_server;
}

int main(int argc, char** argv)
{
  //GOOGLE_PROTOBUF_VERIFY_VERSION;

  ::google::InitGoogleLogging(argv[0]);
  ::testing::InitGoogleTest(&argc, argv);
  ::google::InstallFailureSignalHandler();

  int retval = RUN_ALL_TESTS();

  //::google::protobuf::ShutdownProtobufLibrary();
  ::google::ShutdownGoogleLogging();
  return retval;
}

