#include "glog/logging.h"
#include "gtest/gtest.h"

#include <unistd.h>
#include <stdint.h>
#include <stdlib.h>

// Служебные файлы RTDBUS
#include "exchange_config.hpp"
#include "exchange_config_sac.hpp"
#include "exchange_config_egsa.hpp"

AcquisitionSystemConfig* config = NULL;
const char* config_filename = "BI4500.json";

TEST(TestEXCHANGE, CONFIG)
{
  config = new AcquisitionSystemConfig(config_filename);
  ASSERT_TRUE(1 != 0);

  
  delete config;
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

