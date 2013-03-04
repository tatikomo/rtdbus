#include <string>
#include "gtest/gtest.h"

#include "mdp_worker_api.hpp"
#include "mdp_client_async_api.hpp"

TEST(TestWorker, Creation)
{
  std::string broker = "tcp://localhost:5555";
  std::string service = "echo";
  int verbose = 1;

  mdwrk *worker = new mdwrk(broker, service, verbose);
  EXPECT_TRUE(worker != NULL);
  delete worker;
}

TEST(TestClient, Creation)
{
  int verbose = 1;
  mdcli *client = new mdcli("tcp://localhost:5555", verbose);
  EXPECT_TRUE(client != NULL);
  delete client;
}

int main(int argc, char** argv)
{
  testing::InitGoogleTest(&argc, argv);

  return RUN_ALL_TESTS();
}
