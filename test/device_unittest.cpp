#include <string>
#include "gtest/gtest.h"

#include "zmsg.hpp"
#include "mdp_worker_api.hpp"
#include "mdp_client_async_api.hpp"

TEST(TestUUID, ENCODE)
{
  unsigned char binary_ident[5];
  char * uncoded_ident = NULL;
  zmsg * msg = new zmsg();

  binary_ident[0] = 0x00;
  binary_ident[1] = 0x6B;
  binary_ident[2] = 0x8B;
  binary_ident[3] = 0x45;
  binary_ident[4] = 0x67;

  uncoded_ident = msg->encode_uuid(binary_ident);
  EXPECT_STREQ(uncoded_ident, "@006B8B4567");

  delete msg;
}

TEST(TestUUID, DECODE)
{
  unsigned char binary_ident[5];
  ustring str = "@006B8B4567";
  unsigned char * uncoded_ident = NULL;
  zmsg * msg = new zmsg();

  binary_ident[0] = 0x00;
  binary_ident[1] = 0x6B;
  binary_ident[2] = 0x8B;
  binary_ident[3] = 0x45;
  binary_ident[4] = 0x67;

  uncoded_ident = msg->decode_uuid(str);
  EXPECT_TRUE(uncoded_ident[0] == binary_ident[0]);
  EXPECT_TRUE(uncoded_ident[1] == binary_ident[1]);
  EXPECT_TRUE(uncoded_ident[2] == binary_ident[2]);
  EXPECT_TRUE(uncoded_ident[3] == binary_ident[3]);
  EXPECT_TRUE(uncoded_ident[4] == binary_ident[4]);

  delete msg;
}

TEST(TestHelper, CLOCK)
{
  int64_t before = s_clock();
  sleep (1);
  int64_t after = s_clock();
  // 1000 - одна секунда, добавим 2 мсек для погрешности.
  EXPECT_TRUE(2 > (after - before - 1000));
}

/*TEST(TestWorker, Creation)
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
}*/


int main(int argc, char** argv)
{
  testing::InitGoogleTest(&argc, argv);

  return RUN_ALL_TESTS();
}
