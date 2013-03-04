#include "gtest/gtest.h"

#include "device.hpp"


TEST(TestDevice, Creation)
{
  Device *device = new Device();
  EXPECT_TRUE(device != NULL);
}

int main(int argc, char** argv)
{
  testing::InitGoogleTest(&argc, argv);

  return RUN_ALL_TESTS();
}
