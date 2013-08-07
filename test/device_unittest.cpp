#include <string>
#include "gtest/gtest.h"

#include "helper.hpp"
#include "zmsg.hpp"
#include "mdp_common.h"
#include "mdp_broker.hpp"
#include "mdp_worker_api.hpp"
#include "mdp_client_async_api.hpp"
#include "xdb_database_worker.hpp"
#include "xdb_database_service.hpp"

Broker *broker = NULL;
std::string service_name_1 = "сервис1";
std::string service_name_2 = "service_test_2";
std::string unbelievable_service_name = "unbelievable_service";
std::string worker_identity_1 = "@W000000001";
std::string worker_identity_2 = "@W000000002";
std::string worker_identity_3 = "@W000000003";

Service *service1 = NULL;
Service *service2 = NULL;
int64_t service1_id;
int64_t service2_id;

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
  delete[] uncoded_ident;
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
  delete[] uncoded_ident;
}

TEST(TestHelper, CLOCK)
{
  timer_mark_t now_time;
  timer_mark_t future_time;
  int ret;
  int64_t before = s_clock();

  ret = GetTimerValue(now_time);
  EXPECT_TRUE(ret == 1);

  usleep (500000);

  ret = GetTimerValue(future_time);
  EXPECT_TRUE(ret == 1);

  int64_t after = s_clock();
  // 500 = полсекунды, добавим 2 мсек для погрешности.
  //std::cout << (after - before - 500) << std::endl;
  EXPECT_TRUE(2 > (after - before - 500));

  EXPECT_TRUE (1 >= (future_time.tv_sec - now_time.tv_sec));
  // 1млрд = одна секунда, добавим 100тыс (1 мсек) для погрешности.
  EXPECT_TRUE (100000 > (future_time.tv_nsec - now_time.tv_nsec - 500000000));
}

TEST(TestProxy, RUNTIME)
{
  bool status = false;

  try
  {
    s_version_assert (3, 2);
    status = true;
  }
  catch(...)
  {
    status = false;
  }

  EXPECT_EQ(status, true);
}

TEST(TestProxy, BROKER_CREATION)
{
  bool status = false;

  try
  {
    broker = new Broker(1); // be verbose
    /*
     * NB: "tcp://lo:5555" использует локальный интерфейс, 
     * что удобно для мониторинга wireshark-ом.
     */
    broker->bind ("tcp://*:5555");
    status = true;
  }
  catch (zmq::error_t err)
  {
    std::cout << "E: " << err.what() << std::endl;
    status = false;
  }

  ASSERT_TRUE(broker != NULL);
  EXPECT_EQ(status, true);
}

TEST(TestProxy, BROKER_RUNTIME)
{
  Worker *wrk = NULL;
  bool status = false;
  
  status = broker->Init();
  EXPECT_EQ(status, true);


  // TODO: Зарегистрировать Обработчик в БД
  // Необходимые шаги:
  // 1. Создание Сервиса
  // 2. Регистрация Обработчика для этого Сервиса
  broker->database_snapshot("BROKER_RUNTIME");
  service1 = broker->service_require(service_name_1);
  ASSERT_TRUE(service1 != NULL);

  broker->database_snapshot("BROKER_RUNTIME");
  wrk = broker->worker_require(worker_identity_1);
  ASSERT_TRUE(wrk == NULL); /* Обработчик еще не зарегистрирован */
  delete wrk;

  wrk = broker->worker_register(service_name_1, worker_identity_1);
  ASSERT_TRUE(wrk != NULL);
  delete wrk;

  wrk = broker->worker_require(worker_identity_1);
  ASSERT_TRUE(wrk != NULL); /* Обработчик уже зарегистрирован */
  delete wrk;

  broker->database_snapshot("BROKER_RUNTIME");
}

TEST(TestProxy, BROKER_DELETION)
{
  delete service1;
  delete broker;
}

#if defined FUNCTIONAL_TESTS
/*
 * Должны быть запущены Брокер и ECHO-Сервис
 */
TEST(TestClient, EXCHANGE)
{
  int verbose = 0;
  int count;
  const int MSG_COUNT = 4;
  mdcli *session = NULL;
  zmsg  *reply   = NULL;
  zmsg  *request = NULL;

  session = new mdcli ("tcp://localhost:5555", verbose);
  EXPECT_TRUE(session != NULL);
  
  for (count = 0; count < 4; count++) 
  {
     request = new zmsg ();
     EXPECT_TRUE(request != NULL);

     request->push_front((char*)"Hello world");
     session->send ("echo", request);
     
     reply = session->recv ();
     EXPECT_TRUE(reply != NULL);

     if (reply)
          delete reply;
     else
          break;              //  Interrupt or failure
  }
  EXPECT_TRUE (count == MSG_COUNT);
  delete session;
}

#endif

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
