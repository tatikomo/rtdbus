#include <string>
#include "gtest/gtest.h"
#include "glog/logging.h"
#include <google/protobuf/stubs/common.h>

#include "helper.hpp"
#include "zmsg.hpp"
#include "mdp_common.h"
#include "mdp_broker.hpp"
#include "mdp_worker_api.hpp"
#include "mdp_client_async_api.hpp"
#include "xdb_database_worker.hpp"
#include "xdb_database_broker.hpp"
#include "xdb_database_service.hpp"

#include "proxy.hpp"
#include "wdigger.hpp"
#include "trader.hpp"

// Определения пользовательских сообщений и их сериализованных структур
#include "msg_common.h"
#include "proto/common.pb.h"

Broker *broker = NULL;
std::string service_name_1 = "NYSE";
std::string service_name_2 = "service_test_2";
std::string unbelievable_service_name = "unbelievable_service";
std::string worker_identity_1 = "@W000000001";
std::string worker_identity_2 = "@W000000002";
std::string worker_identity_3 = "@W000000003";
std::string client_identity_1 = "@C000000001";

XDBDatabaseBroker *database = NULL;

const char *attributes_connection_to_broker = "tcp://localhost:5555";
Service *service1 = NULL;
Service *service2 = NULL;
Worker  *worker = NULL;
int64_t service1_id;
int64_t service2_id;

void PrintWorker(Worker*);

void PrintWorker(Worker* worker)
{
  if (!worker)
    return;
  std::cout << worker->GetID()<<":"<<worker->GetIDENTITY()
    <<"/"<<worker->GetSTATE()<<"/"<<worker->Expired()<<std::endl;

}

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
  std::string str = "@006B8B4567";
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
  timer_mark_t now_plus_time;
  timer_mark_t future_time;
  int ret;
  int64_t before = s_clock();
  // Проверка "просроченности" данных об Обработчике
  Worker* worker = new Worker();
  ASSERT_TRUE(worker);

  ret = GetTimerValue(now_time);
  EXPECT_TRUE(ret == 1);

  memcpy(&now_plus_time, &now_time, sizeof(timer_mark_t));
  now_plus_time.tv_nsec = 1000000;
  worker->SetEXPIRATION(now_plus_time);
  EXPECT_TRUE(worker->Expired() == false);

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

  usleep(500000);

  EXPECT_TRUE(worker->Expired() == true);
  delete worker;
}

TEST(TestProxy, RUNTIME_VERSION)
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

/*
 * Проверить следующие методы класса Broker()
 */
#if 0
   // Регистрировать новый экземпляр Обработчика для Сервиса
   //  ---------------------------------------------------------------------
+  Worker * worker_register(const std::string&, const std::string&);

   //  ---------------------------------------------------------------------
   //  Locate or create new service entry
+  Service * service_require (const std::string& name);

   //  ---------------------------------------------------------------------
   //  Dispatch requests to waiting workers as possible
+  void service_dispatch (Service *srv/*, zmsg *msg*/);

   //  ---------------------------------------------------------------------
   //  Handle internal service according to 8/MMI specification
   void service_internal (const std::string& service_name, zmsg *msg);

   //  ---------------------------------------------------------------------
   //  Creates worker if necessary
+  Worker * worker_require (const std::string& sender/*, char* identity*/);

   //  ---------------------------------------------------------------------
   //  Deletes worker from all data structures, and destroys worker
+  void worker_delete (Worker *&wrk, int disconnect);

   //  ---------------------------------------------------------------------
   //  Processes one READY, REPORT, HEARTBEAT or  DISCONNECT message 
   //  sent to the broker by a worker
   void worker_msg (const std::string& sender, zmsg *msg);

   //  ---------------------------------------------------------------------
   //  Send message to worker
   //  If pointer to message is provided, sends that message
   void worker_send (Worker*, const char*, const std::string&, zmsg *msg);
   void worker_send (Worker*, const char*, const std::string&, Letter*);

   //  ---------------------------------------------------------------------
   //  This worker is now waiting for work
   void worker_waiting (Worker *worker);

   //  ---------------------------------------------------------------------
   //  Process a request coming from a client
   void client_msg (const std::string& sender, zmsg *msg);
#endif
/*
 * Конец списка проверяемых методов класса Broker
 */


/*
 * Рабочий цикл клиентской программы TRADER
 * Тестируется класс: mdcli
 * Используемая служба: NYSE (service_name_1)
 */
static void *
client_task (void *args)
{
  int       verbose   = 1;
  char    * s_price   = NULL;
  zmsg    * request   = NULL;
  zmsg    * report    = NULL;
  Trader  * client    = new Trader (attributes_connection_to_broker, verbose);
  int       count;

  try
  {
    s_price = new char[10];
    //  Send 100 sell orders
    for (count = 0; count < 2; count++) {
        request = new zmsg ();
        request->push_front ((char*)"8");    // volume
        sprintf(s_price, "%d", count + 1000);
        request->push_front ((char*)s_price);
        request->push_front ((char*)"SELL");
        client->send (service_name_1.c_str(), request);
        delete request;
    }

    //  Send 1 buy order.
    //  This order will match all sell orders.
    request = new zmsg ();
    request->push_front ((char*)"800");      // volume
    request->push_front ((char*)"2000");     // price
    request->push_front ((char*)"BUY");
    client->send (service_name_1.c_str(), request);
    delete request;

    //  Wait for all trading reports
    while (1) {
        report = client->recv ();
        if (report == NULL)
            break;
        printf("client recived this:");
        report->dump();
        assert (report->parts () >= 2);
        
        std::string report_type = report->pop_front ();
        std::string volume      = report->pop_front ();

        printf ("%s %s shares\n",  report_type.c_str(), volume.c_str());
        delete report;
    }
  }
  catch (zmq::error_t err)
  {
      std::cout << "E: " << err.what() << std::endl;
  }
  delete[] s_price;
  delete client;

  pthread_exit(NULL);
}

/*
 * Рабочий цикл программы-Обработчика Digger для службы NYSE
 * Тестируется класс mdwrk
 */
static void *
worker_task (void *args)
{
  int verbose = 1;

  try
  {
    Digger *engine = new Digger(attributes_connection_to_broker,
                                service_name_1.c_str(),
                                verbose);
    while (!s_interrupted) 
    {
       std::string * reply_to = new std::string;
       zmsg        * request  = NULL;

       request = engine->recv (reply_to);
       if (request)
       {
         engine->handle_request (request, reply_to);
         delete reply_to;
       }
       else
         break;          // Worker has been interrupted
    }
    delete engine;
  }
  catch(zmq::error_t err)
  {
    std::cout << "E: " << err.what() << std::endl;
  }

  pthread_exit(NULL);
}

/*
 * Используется для проведения функциональных тестов:
 * 1. Работа с Обработчиками
 * 2. Работа с Клиентами
 * NB: Не использует глобальных переменных
 */
static void *
broker_task (void *args)
{
  int verbose = 1;
  std::string sender;
  std::string empty;
  std::string header;
  Broker *broker = NULL;

  try
  {
     s_version_assert (3, 2);
     s_catch_signals ();
     broker = new Broker(verbose);
     /*
      * NB: "tcp://lo:5555" использует локальный интерфейс, 
      * что удобно для мониторинга wireshark-ом.
      */
     broker->bind ("tcp://*:5555");

     broker->start_brokering();
  }
  catch (zmq::error_t err)
  {
     std::cout << "E: " << err.what() << std::endl;
  }

  if (s_interrupted)
  {
     printf ("W: interrupt received, shutting down...\n");
  }

  delete broker;

  pthread_exit(NULL);
}

/*
 * Этот тест используется для проверки внутренних методов класса Broker;
 * Все функциональные тесты проверяются с экземпляром, созданным в треде broker_task;
 * NB: Использует глобальную переменную broker, объект необходимо удалить позже;
 */
TEST(TestProxy, BROKER_INTERNAL)
{
  Worker *wrk = NULL;
  bool status = false;

  broker = new Broker(true); // be verbose
  ASSERT_TRUE (broker != NULL);
  /*
   * NB: "tcp://lo:5555" использует локальный интерфейс, 
   * что удобно для мониторинга wireshark-ом.
   */

  status = broker->bind ("tcp://*:5555");
  EXPECT_EQ(status, true);

  status = broker->Init();
  EXPECT_EQ(status, true);

  // TODO: Зарегистрировать Обработчик в БД
  // Необходимые шаги:
  // 1. Создание Сервиса
  // 2. Регистрация Обработчика для этого Сервиса
  broker->database_snapshot("BROKER_INTERNAL");
  service1 = broker->service_require(service_name_1);
  ASSERT_TRUE(service1 != NULL);
  // Сервис успешно зарегистрировался

  broker->database_snapshot("BROKER_INTERNAL");
  wrk = broker->worker_require(worker_identity_1);
  ASSERT_TRUE(wrk == NULL); /* Обработчик еще не зарегистрирован */
  delete wrk;

  wrk = broker->worker_register(service_name_1, worker_identity_1);
  ASSERT_TRUE(wrk != NULL);
  delete wrk;
  // Обработчик успешно зарегистрирован

  wrk = broker->worker_require(worker_identity_1);
  ASSERT_TRUE(wrk != NULL); /* Обработчик уже зарегистрирован */
  // не удалять пока wrk

  broker->database_snapshot("BROKER_INTERNAL");

  /*
   * Обработчик wrk будет удален внутри функции
   * Он останется в БД, но его состояние сменится с ARMED(1) на DISARMED(0)
   * Для проверки следует сравнить срезы БД до и после функции worker_delete
   */
  broker->worker_delete(wrk, 0);
  broker->database_snapshot("BROKER_INTERNAL");
  wrk = broker->worker_require(worker_identity_1);
  ASSERT_TRUE(wrk != NULL); /* Обработчик зарегистрирован, не активирован */
  EXPECT_EQ(wrk->GetSTATE(), Worker::DISARMED);
//  PrintWorker(wrk);
}

TEST(TestProxy, PURGE_WORKERS)
{
  ServiceList  *sl = broker->get_internal_db_api()->GetServiceList();
  Service      *service = sl->first();
  Worker       *wrk = NULL;
  int           srv_count;
  int           wrk_count;

  /*
   * Добавить Сервису второго Обработчика
   */
  wrk = broker->worker_register(service_name_1, worker_identity_2);
  ASSERT_TRUE(wrk != NULL);
  delete wrk;
  // Обработчик успешно зарегистрирован
  // теперь в базе должно быть два Обработчика, worker_identity_1 и worker_identity_2

  srv_count = 0;
  while(NULL != service)
  {
    wrk_count = 0;
    /* Пройти по списку Сервисов */
    while (NULL != (wrk = broker->get_internal_db_api()->PopWorker(service)))
    {
        LOG(INFO) <<srv_count<<":"<<service->GetNAME()<<" => "<<wrk_count<<":"<<wrk->GetIDENTITY();
        if (wrk->Expired ()) 
        {
          LOG(INFO) << "Deleting expired worker: " << wrk->GetIDENTITY();
          broker->worker_delete (wrk, 0); // Обработчик не подавал признаков жизни
        }
        else delete wrk; // Обработчик жив, просто освободить память
        wrk_count++;
    }
    service = sl->next();
    srv_count++;
  }
}

/*
 * Проверить поступление сообщений от Клиентов
 *
 * Структура сообщений от Клиента:
 * [011] Идентификатор Клиента
 * [000]
 * [006] MDPC0X
 * [0XX] Имя Сервиса
 * [0XX] Сериализованный заголовок
 * [XXX] Сериализованное тело сообщения
*/
#if 1
TEST(TestProxy, CLIENT_MESSAGE)
{
  zmsg*             msg = NULL;
  RTDBM::Header     pb_header;
  RTDBM::ExecResult pb_exec_result_request;
  std::string       pb_serialized_header;
  std::string       pb_serialized_request;

  pb_header.set_protocol_version(1);
  pb_header.set_exchange_id(9999999);
  pb_header.set_source_pid(9999);
  pb_header.set_proc_dest("В чащах юга жил-был Цитрус?");
  pb_header.set_proc_origin("Да! Но фальшивый экземпляр.");
  pb_header.set_sys_msg_type(100);
  pb_header.set_usr_msg_type(ADG_D_MSG_EXECRESULT);

  pb_exec_result_request.set_user_exchange_id(9999999);
  pb_exec_result_request.set_exec_result(23145);
  pb_exec_result_request.set_failure_cause(5);

  pb_header.SerializeToString(&pb_serialized_header);
  pb_exec_result_request.SerializeToString(&pb_serialized_request);

  msg = new zmsg();
  ASSERT_TRUE(msg);

  msg->push_front(pb_serialized_request);
  msg->push_front(pb_serialized_header);
  msg->push_front(service_name_1);

  // msg удаляется внутри
  broker->client_msg(client_identity_1, msg);
  broker->database_snapshot("BROKER_INTERNAL");
}
#endif

/*
 * Проверить передачу Клиентсткого сообщения Обработчику
 * У клиенсткого cообщения, отправляемого Брокером Обработчику, д.б. 4 фрейма:
 * 1. идентификатор Клиента
 * 2. пустой фрейм
 * 3. сериализованный заголовок
 * 4. сериализованное тело сообщения
 */
TEST(TestProxy, SERVICE_DISPATCH)
{
  zmsg*             msg = NULL;
  RTDBM::Header     pb_header;
  RTDBM::ExecResult pb_exec_result_request;
  std::string       pb_serialized_header;
  std::string       pb_serialized_request;

  // Зарегистрировать нового Обработчика worker_identity_1 для service_name_1
  worker = broker->worker_register(service_name_1, worker_identity_1);
  ASSERT_TRUE(worker != NULL);
  broker->database_snapshot("BROKER_INTERNAL");

  pb_header.set_protocol_version(1);
  pb_header.set_exchange_id(9999999);
  pb_header.set_source_pid(9999);
  pb_header.set_proc_dest("В чащах юга жил-был Цитрус?");
  pb_header.set_proc_origin("Да! Но фальшивый экземпляр.");
  pb_header.set_sys_msg_type(100);
  pb_header.set_usr_msg_type(ADG_D_MSG_EXECRESULT);

  pb_exec_result_request.set_user_exchange_id(9999999);
  pb_exec_result_request.set_exec_result(23145);
  pb_exec_result_request.set_failure_cause(5);

  pb_header.SerializeToString(&pb_serialized_header);
  pb_exec_result_request.SerializeToString(&pb_serialized_request);

  msg = new zmsg();
  ASSERT_TRUE(msg);

  msg->push_front(pb_serialized_request);
  msg->push_front(pb_serialized_header);

  broker->database_snapshot("BROKER_INTERNAL");
  // service1 удаляется внутри
  broker->service_dispatch(service1, msg);
 // delete service1;
  broker->database_snapshot("BROKER_INTERNAL");

  delete msg;
}


TEST(TestProxy, BROKER_DELETE)
{
  delete broker;
}

#if 0
TEST(TestProxy, BROKER_RUNTIME)
{
    pthread_t broker;
    pthread_t worker;
    pthread_t client;

    /* Создать экземпляр Брокера */
    pthread_create (&broker, NULL, broker_task, NULL);
    sleep(1);

    /* Создать одного Обработчика службы NYSE */
    pthread_create (&worker, NULL, worker_task, NULL);
    sleep(1);

    /* Создать одного клиента службы NYSE */
    pthread_create (&client, NULL, client_task, NULL);

    /* Дождаться завершения работы клиента */
    pthread_join (client, NULL);

    /* Остановить Обработчика NYSE */
    int cw = pthread_cancel(worker);
    sleep(1);

    /* Остановить Брокера */
    int cb = pthread_cancel(broker);
    printf("%d %d\n", cw, cb);
}
#endif

int main(int argc, char** argv)
{
  ::testing::InitGoogleTest(&argc, argv);
  ::google::InstallFailureSignalHandler();
  ::google::InitGoogleLogging(argv[0]);
  int retval = RUN_ALL_TESTS();
  ::google::protobuf::ShutdownProtobufLibrary();
  ::google::ShutdownGoogleLogging();
  return retval;
}
