#include <string>
#include "gtest/gtest.h"
#include "glog/logging.h"
#include "google/protobuf/stubs/common.h"

#include "helper.hpp"

#include "mdp_zmsg.hpp"
#include "mdp_common.h"
#include "mdp_broker.hpp"
#include "mdp_worker_api.hpp"
#include "mdp_letter.hpp"
#include "mdp_client_async_api.hpp"
#include "mdp_proxy.hpp"

#include "xdb_broker.hpp"
#include "xdb_impl_db_broker.hpp"
//#include "xdb_broker_letter.hpp"
//#include "xdb_broker_service.hpp"

using namespace xdb;
#include "wdigger.hpp"
#include "pulsar.hpp"

// Определения пользовательских сообщений и их сериализованных структур
#include "msg_common.h"
#include "proto/common.pb.h"

mdp::Digger *digger = NULL;
mdp::Broker *broker = NULL;
std::string service_name_1 = "NYSE";
std::string service_name_2 = "service_test_2";
std::string unbelievable_service_name = "unbelievable_service";
std::string worker_identity_1 = "@W000000001";
std::string worker_identity_2 = "@W000000002";
std::string worker_identity_3 = "@W000000003";
std::string client_identity_1 = "@C000000001";
std::string client_identity_2 = "@C000000002";

xdb::DatabaseBroker *database = NULL;

const char *attributes_connection_to_broker = "tcp://localhost:5555";
xdb::Service *service1 = NULL;
xdb::Service *service2 = NULL;
xdb::Worker  *worker = NULL;
int64_t service1_id;
int64_t service2_id;

/*
 * Прототипы функций
 */
void         Dump(mdp::Letter*);
void         PrintWorker(xdb::Worker*);
xdb::Letter* GetNewLetter(rtdbMsgType, rtdbExchangeId);

/*
 * Реализация функций
 */
void PrintWorker(xdb::Worker* worker)
{
  if (!worker)
    return;
  std::cout << worker->GetID()<<":"<<worker->GetIDENTITY()
    <<"/"<<worker->GetSTATE()<<"/"<<worker->Expired()<<std::endl;
}

xdb::Letter* GetNewLetter(rtdbMsgType msg_type, rtdbExchangeId user_exchange)
{
  ::google::protobuf::Message* pb_message = NULL;
  std::string   pb_serialized_request;
  xdb::Letter  *xdb_letter;
  mdp::Letter  *mdp_letter;

  switch(msg_type)
  {
    case ADG_D_MSG_EXECRESULT:
        pb_message = new RTDBM::ExecResult;
        static_cast<RTDBM::ExecResult*>(pb_message)->set_user_exchange_id(user_exchange);
        static_cast<RTDBM::ExecResult*>(pb_message)->set_exec_result(1);
        static_cast<RTDBM::ExecResult*>(pb_message)->set_failure_cause(0);
        break;

    case ADG_D_MSG_ASKLIFE:
        pb_message = new RTDBM::AskLife;
        static_cast<RTDBM::AskLife*>(pb_message)->set_user_exchange_id(user_exchange);
        break;

    default:
        std::cout << "Unsupported message type: "<< msg_type;
        break;
  }

  pb_message->SerializeToString(&pb_serialized_request);
  delete pb_message;
  mdp_letter = new mdp::Letter(msg_type, 
                               static_cast<const std::string&>(service_name_1),
                               static_cast<const std::string*>(&pb_serialized_request));

  xdb_letter = new xdb::Letter(client_identity_2.c_str(),
                           /* заголовок */
                           mdp_letter->SerializedHeader(),
                           /* тело сообщения */
                           mdp_letter->SerializedData());
  delete mdp_letter;

  return xdb_letter;
}

Pulsar::Pulsar(std::string broker, int verbose) : mdcli(broker, verbose)
{
  LOG(INFO) << "Create pulsar instance";
}

void purge_workers()
{
  xdb::ServiceList  *sl = broker->get_internal_db_api()->GetServiceList();
  xdb::Service      *service = sl->first();
  xdb::Worker       *wrk = NULL;

  broker->database_snapshot("TEST_PURGE_WORKERS.START");
  while(NULL != service)
  {
    /* Пройти по списку Сервисов */
    /*while (NULL != (*/wrk = broker->get_internal_db_api()->PopWorker(service);/*))*/
    if (wrk)
    {
        LOG(INFO) <<service->GetID()<<":"<<service->GetNAME()<<" => "<<wrk->GetIDENTITY();
        if (wrk->Expired ()) 
        {
          LOG(INFO) << "Deleting expired worker: " << wrk->GetIDENTITY();
          broker->release (wrk, 0); // Обработчик не подавал признаков жизни
        }
        delete wrk; // Обработчик жив, просто освободить память
    }
    service = sl->next();
  }
  broker->database_snapshot("TEST_PURGE_WORKERS.STOP");
}

////////////////////////////////////////////////////////////////////////////////
TEST(TestUUID, ENCODE)
{
  unsigned char binary_ident[5];
  char * uncoded_ident = NULL;
  mdp::zmsg * msg = new mdp::zmsg();

  LOG(INFO) << "TestUUID ENCODE start";
  binary_ident[0] = 0x00;
  binary_ident[1] = 0x6B;
  binary_ident[2] = 0x8B;
  binary_ident[3] = 0x45;
  binary_ident[4] = 0x67;

  uncoded_ident = msg->encode_uuid(binary_ident);
  EXPECT_STREQ(uncoded_ident, "@006B8B4567");

  delete msg;
  delete[] uncoded_ident;
  LOG(INFO) << "TestUUID ENCODE stop";
}

////////////////////////////////////////////////////////////////////////////////
TEST(TestUUID, DECODE)
{
  unsigned char binary_ident[5];
  std::string str = "@006B8B4567";
  unsigned char * uncoded_ident = NULL;
  mdp::zmsg * msg = new mdp::zmsg();

  LOG(INFO) << "TestUUID DECODE start";
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
  LOG(INFO) << "TestUUID DECODE stop";
}

////////////////////////////////////////////////////////////////////////////////
TEST(TestHelper, CLOCK)
{
  timer_mark_t now_time;
  timer_mark_t now_plus_time;
  timer_mark_t future_time;
  int ret;
  int64_t before = s_clock();
  // Проверка "просроченности" данных об Обработчике
  xdb::Worker* worker = new xdb::Worker();
  ASSERT_TRUE(worker);
  LOG(INFO) << "TestHelper CLOCK start";

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
  // 1млрд = одна секунда, добавим 200тыс (2 мсек) для погрешности.
  EXPECT_TRUE (200000 > 
        ((future_time.tv_nsec + future_time.tv_sec * 1.0e9) - (now_time.tv_nsec + now_time.tv_sec * 1.0e9)
          - 5.0e8));

  usleep(500000);

  EXPECT_TRUE(worker->Expired() == true);
  delete worker;
  LOG(INFO) << "TestHelper CLOCK stop";
}

void Dump(mdp::Letter* letter)
{
  RTDBM::ExecResult *kokoko = static_cast<RTDBM::ExecResult*>(letter->data());

  std::cout << (int)letter->header().get_protocol_version();
  std::cout << "/" << letter->header().get_exchange_id();
  std::cout << "/" << letter->header().get_source_pid();
  std::cout << "/" << letter->header().get_proc_dest();
  std::cout << "/" << letter->header().get_proc_origin();
  std::cout << "/" << letter->header().get_sys_msg_type();
  std::cout << "/" << letter->header().get_usr_msg_type();
  std::cout << "[" << kokoko->user_exchange_id();
  std::cout << "/" << kokoko->exec_result();
  std::cout << "/" << kokoko->failure_cause()<<"]";
  std::cout << std::endl;
}

////////////////////////////////////////////////////////////////////////////////
TEST(TestLetter, USAGE)
{
  mdp::Letter* letter1 = NULL;
  mdp::Letter* letter2 = NULL;
  RTDBM::ExecResult pb_exec_result_request;
  std::string       pb_serialized_header;
  std::string       pb_serialized_request;

  pb_exec_result_request.set_user_exchange_id(102);
  pb_exec_result_request.set_exec_result(1);
  pb_exec_result_request.set_failure_cause(0);
  pb_exec_result_request.SerializeToString(&pb_serialized_request);  

  letter1 = new mdp::Letter(ADG_D_MSG_EXECRESULT, "NYSE", &pb_serialized_request);
  Dump(letter1);

  letter2 = new mdp::Letter(ADG_D_MSG_EXECRESULT, "NYSE");
  RTDBM::ExecResult* exec_result = static_cast<RTDBM::ExecResult*>(letter2->mutable_data());
  exec_result->set_user_exchange_id(103);
  exec_result->set_exec_result(104);
  exec_result->set_failure_cause(105);
  Dump(letter2);

  pb_exec_result_request.ParseFromString(letter2->SerializedData());
  ASSERT_EQ(pb_exec_result_request.user_exchange_id(), 103);
  ASSERT_EQ(pb_exec_result_request.exec_result(), 104);
  ASSERT_EQ(pb_exec_result_request.failure_cause(), 105);

  delete letter1;
  delete letter2;

  // TODO: создать экземпляры остальных типов сообщений
}

////////////////////////////////////////////////////////////////////////////////
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
/*
   // Регистрировать новый экземпляр Обработчика для Сервиса
   //  ---------------------------------------------------------------------
+  Worker * worker_register(const std::string&, const std::string&);

   //  ---------------------------------------------------------------------
   //  Locate or create new service entry
+  Service * service_require (const std::string& name);

   //  ---------------------------------------------------------------------
   //  Dispatch requests to waiting workers as possible
+  void service_dispatch (Service *srv);//, zmsg *msg

   //  ---------------------------------------------------------------------
   //  Handle internal service according to 8/MMI specification
   void service_internal (const std::string& service_name, zmsg *msg);

   //  ---------------------------------------------------------------------
   //  Creates worker if necessary
+  Worker * worker_require (const std::string& sender);//, char* identity

   //  ---------------------------------------------------------------------
   //  Deletes worker from all data structures, and destroys worker
+  !void worker_delete (Worker *&wrk, int disconnect);
   заменено на 
+  void release (Worker *&wrk, int disconnect);

   //  ---------------------------------------------------------------------
   //  Processes one READY, REPORT, HEARTBEAT or  DISCONNECT message 
   //  sent to the broker by a worker
   void worker_msg (const std::string& sender, zmsg *msg);

   //  ---------------------------------------------------------------------
   //  Send message to worker
   //  If pointer to message is provided, sends that message
   void worker_send (Worker*, const char*, const std::string&, zmsg *msg);
   void worker_send (Worker*, const char*, const std::string&, xdb::Letter*);

   //  ---------------------------------------------------------------------
   //  This worker is now waiting for work
   void worker_waiting (Worker *worker);

   //  ---------------------------------------------------------------------
   //  Process a request coming from a client
+   void client_msg (const std::string& sender, zmsg *msg);
*/
#endif
/*
 * Конец списка проверяемых методов класса Broker
 */


/*
 * Рабочий цикл клиентской программы PULSAR
 * Тестируется класс: mdcli
 * Используемая служба: NYSE (service_name_1)
 */
static void *
client_task (void* /*args*/)
{
  int        verbose   = 1;
  mdp::zmsg *request   = NULL;
  mdp::zmsg *report    = NULL;
  Pulsar    *client    = new Pulsar (attributes_connection_to_broker, verbose);
  int        count;
  std::string       pb_serialized_request;
  RTDBM::AskLife    pb_request_asklife;
  RTDBM::ExecResult pb_responce_exec_result;
  mdp::Letter      *mdp_letter;
  const std::string dest = "NYSE";
  static int        user_exchange_id = 0;

  LOG(INFO) << "Start client thread";
  try
  {
    mdp_letter = new mdp::Letter(ADG_D_MSG_ASKLIFE, dest);

    //  Send 10 ADG_D_MSG_ASKLIFE orders
    for (user_exchange_id = 0; user_exchange_id < 10; user_exchange_id++) {
        request = new mdp::zmsg ();

        // Единственное поле, которое может устанавливать клиент
        // напрямую в ADG_D_MSG_ASKLIFE
        static_cast<RTDBM::AskLife*>(mdp_letter->mutable_data())->set_user_exchange_id(user_exchange_id);
        request->push_front(const_cast<std::string&>(mdp_letter->SerializedData()));
        request->push_front(const_cast<std::string&>(mdp_letter->SerializedHeader()));
        LOG(INFO) << "Send message id="<<user_exchange_id<<" from client";
        client->send (service_name_1.c_str(), request);
        delete request;
    }
    delete mdp_letter;

    EXPECT_EQ(user_exchange_id, 10);
    count = 0;

    //  Wait for all trading reports
    while (1) {
        report = client->recv ();
        if (report == NULL)
            break;
        count++;
        LOG(INFO) << "Receive message id="<<count<<" from worker";
        report->dump();
        assert (report->parts () >= 2);
        
        mdp_letter = new mdp::Letter(report);
        Dump(mdp_letter);

        delete mdp_letter;
        delete report;
    }
    // Приняли ровно столько, сколько ранее отправили
    EXPECT_TRUE(count == user_exchange_id);
  }
  catch (zmq::error_t err)
  {
      std::cout << "E: " << err.what() << std::endl;
  }
  delete client;

  // Завершить исполнение Брокера - все сообщения отправлены/получены
  s_interrupted = 1;
  LOG(INFO) << "Stop client thread";
  pthread_exit(NULL);
}

/*
 * Рабочий цикл программы-Обработчика Digger для службы NYSE
 * Тестируется класс mdwrk
 */
static void *
worker_task (void* /*args*/)
{
  int verbose = 1;

  LOG(INFO) << "Start worker thread";
  try
  {
    mdp::Digger *engine = new mdp::Digger(attributes_connection_to_broker,
                                service_name_1.c_str(),
                                verbose);
    while (!s_interrupted) 
    {
       std::string *reply_to = new std::string;
       mdp::zmsg   *request  = NULL;

       request = engine->recv (reply_to);
       LOG(INFO) << "Receive message "<<request<<" from client";
       if (request)
       {
         engine->handle_request (request, reply_to);
         delete request;
       }
       delete reply_to;

       if (!request)
         break;       // Worker has been interrupted
    }
    delete engine;
  }
  catch(zmq::error_t err)
  {
    std::cout << "E: " << err.what() << std::endl;
  }

  LOG(INFO) << "Stop worker thread";
  pthread_exit(NULL);
}

/*
 * Используется для проведения функциональных тестов:
 * 1. Работа с Обработчиками
 * 2. Работа с Клиентами
 * NB: Не использует глобальных переменных
 */
static void *
broker_task (void* /*args*/)
{
  int verbose = 1;
  std::string sender;
  std::string empty;
  std::string header;
  mdp::Broker *broker = NULL;

  LOG(INFO) << "Start broker thread";
  try
  {
     s_version_assert (3, 2);
     s_catch_signals ();
     broker = new mdp::Broker(verbose);
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
     LOG(WARNING)<<"Interrupt received, shutting down...";
  }

  LOG(INFO) << "Stop broker thread";
  delete broker;

  pthread_exit(NULL);
}

////////////////////////////////////////////////////////////////////////////////
/*
 * Этот тест используется для проверки внутренних методов класса Broker;
 * Все функциональные тесты проверяются с экземпляром, созданным в треде broker_task;
 * NB: Использует глобальную переменную broker, объект необходимо удалить позже;
 */
TEST(TestProxy, BROKER_INTERNAL)
{
  xdb::Worker *wrk = NULL;
  bool status = false;

  LOG(INFO) << "TestProxy BROKER_INTERNAL start";
  broker = new mdp::Broker(true); // be verbose
  ASSERT_TRUE (broker != NULL);
  /*
   * NB: "tcp://lo:5555" использует локальный интерфейс, 
   * что удобно для мониторинга wireshark-ом.
   */

  status = broker->bind ("tcp://*:5555");
  EXPECT_EQ(status, true);

  status = broker->Init();
  EXPECT_EQ(status, true);

  // Зарегистрировать Обработчик в БД
  // 1. Создание Сервиса
  // 2. Регистрация Обработчика для этого Сервиса
  broker->database_snapshot("TEST_BROKER_INTERNAL.START");
  service1 = broker->service_require(service_name_1);
  ASSERT_TRUE(service1 != NULL);
  // Сервис успешно зарегистрировался

  broker->database_snapshot("TEST_BROKER_INTERNAL.PROCESS");
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

  broker->database_snapshot("TEST_BROKER_INTERNAL.PROCESS");

  /*
   * Обработчик останется в БД, но его состояние сменится с ARMED(1) на DISARMED(0)
   * Для проверки следует сравнить срезы БД до и после функции release()
   */
  broker->release(wrk, 0);
  delete wrk;
  broker->database_snapshot("TEST_BROKER_INTERNAL.PROCESS");

  wrk = broker->worker_require(worker_identity_1);
  ASSERT_TRUE(wrk != NULL); /* Обработчик зарегистрирован, не активирован */
  EXPECT_EQ(wrk->GetSTATE(), xdb::Worker::DISARMED);

  broker->get_internal_db_api()->SetWorkerState(wrk, xdb::Worker::ARMED);
  EXPECT_EQ(wrk->GetSTATE(), xdb::Worker::ARMED);
  broker->database_snapshot("TEST_BROKER_INTERNAL.STOP");
  PrintWorker(wrk);
  delete wrk;
  LOG(INFO) << "TestProxy BROKER_INTERNAL stop";
}


/* 
 * Проверить механизм массового занесения сообщений в спул Службы, без привязки 
 * к Обработчикам.
 *
 * NB: service1 уже существует
 */
TEST(TestProxy, PUSH_REQUEST)
{
  xdb::Letter *letter;
  bool status;

  LOG(INFO) << "TestProxy PUSH_REQUEST start";
  broker->database_snapshot("TEST_PUSH_REQUEST.START");
  for (int idx=1; idx<20; idx++)
  {
      letter = GetNewLetter(ADG_D_MSG_ASKLIFE, idx);
      status = broker->get_internal_db_api()->PushRequestToService(service1, letter);
      EXPECT_EQ(status, true);
      delete letter;
  }
  broker->database_snapshot("TEST_PUSH_REQUEST.STOP");
  LOG(INFO) << "TestProxy PUSH_REQUEST stop";
}

////////////////////////////////////////////////////////////////////////////////
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
TEST(TestProxy, CLIENT_MESSAGE)
{
  mdp::zmsg        *msg = NULL;
  RTDBM::Header     pb_header;
  RTDBM::ExecResult pb_exec_result_request;
  std::string       pb_serialized_header;
  std::string       pb_serialized_request;

  LOG(INFO) << "TestProxy CLIENT_MESSAGE start";
  broker->database_snapshot("TEST_CLIENT_MESSAGE.START");
  pb_header.set_protocol_version(1);
  pb_header.set_exchange_id(1);
  pb_header.set_source_pid(getpid());
  pb_header.set_proc_dest("dest");
  pb_header.set_proc_origin("src");
  pb_header.set_sys_msg_type(100);
  pb_header.set_usr_msg_type(ADG_D_MSG_EXECRESULT);

  pb_exec_result_request.set_user_exchange_id(1);
  pb_exec_result_request.set_exec_result(1);
  pb_exec_result_request.set_failure_cause(1);

  pb_header.SerializeToString(&pb_serialized_header);
  pb_exec_result_request.SerializeToString(&pb_serialized_request);

  msg = new mdp::zmsg();
  ASSERT_TRUE(msg);

  msg->push_front(pb_serialized_request);
  msg->push_front(pb_serialized_header);
  msg->push_front(service_name_1);

  // msg удаляется внутри
  broker->client_msg(client_identity_1, msg);
  broker->database_snapshot("TEST_CLIENT_MESSAGE.STOP");
  LOG(INFO) << "TestProxy CLIENT_MESSAGET stop";
}

#if 0
//Этот код уже проверяется в CLIENT_MESSAGE
////////////////////////////////////////////////////////////////////////////////
TEST(TestProxy, WAITING_LETTER)
{
  int processed_messages_count = 0;
  xdb::Letter       *letter = NULL;
  xdb::ServiceList  *sl = broker->get_internal_db_api()->GetServiceList();
  xdb::Service      *service = sl->first();
  xdb::Worker       *wrk = NULL;

  broker->database_snapshot("WAITING_LETTER");

  wrk = broker->worker_require(worker_identity_1);
  ASSERT_TRUE(wrk != NULL); /* Обработчик зарегистрирован, активировать */
  broker->get_internal_db_api()->SetWorkerState(wrk, xdb::Worker::ARMED);
  EXPECT_EQ(wrk->GetSTATE(), xdb::Worker::ARMED);

  broker->database_snapshot("WAITING_LETTER");
  while(NULL != service)
  {

    while (NULL != (wrk = broker->get_internal_db_api()->PopWorker(service)))
    {
      LOG(INFO) << "Pop worker '"<<wrk->GetIDENTITY()<<"' with state "<<wrk->GetSTATE();
      while (NULL != (letter = broker->get_internal_db_api()->GetWaitingLetter(service)))
      {
        // Передать ожидающую обслуживания команду выбранному Обработчику
        broker->worker_send (wrk, (char*)MDPW_REQUEST, EMPTY_FRAME, letter);
        processed_messages_count++;
      }
      delete wrk;
    }
    service = sl->next();
  }
  EXPECT_EQ(processed_messages_count, 1);
  broker->database_snapshot("WAITING_LETTER");
}
#endif

////////////////////////////////////////////////////////////////////////////////
TEST(TestProxy, PURGE_WORKERS)
{
  xdb::Worker       *wrk = NULL;
  timer_mark_t mark = { 0, 0 };

  LOG(INFO) << "TestProxy PURGE_WORKERS start";
  broker->database_snapshot("TEST_PURGE_WORKERS.START");

  // Добавить Сервису второго Обработчика
  wrk = broker->worker_register(service_name_1, worker_identity_2);
  ASSERT_TRUE(wrk != NULL);

  // Обработчик успешно зарегистрирован.
  // Теперь в базе должно быть два Обработчика:
  // worker_identity_1(DISARMED) и worker_identity_2(ARMED)
  purge_workers();
  // На выходе не должно быть удаленных Обработчиков


  // Установить срок годности Обработчика wrk "в прошлое"
  // и повторить проверку. На этот раз один обработчик д.б. удален
  wrk->SetEXPIRATION(mark);
  delete wrk;
  purge_workers();
  broker->database_snapshot("TEST_PURGE_WORKERS.STOP");
  LOG(INFO) << "TestProxy PURGE_WORKERS stop";
}

////////////////////////////////////////////////////////////////////////////////
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
  mdp::zmsg        *msg = NULL;
  RTDBM::Header     pb_header;
  RTDBM::ExecResult pb_exec_result_request;
  std::string       pb_serialized_header;
  std::string       pb_serialized_request;

  LOG(INFO) << "TestProxy SERVICE_DISPATCH start";
  // Зарегистрировать нового Обработчика worker_identity_1 для service_name_1
  worker = broker->worker_register(service_name_1, worker_identity_1);
  ASSERT_TRUE(worker != NULL);

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

  msg = new mdp::zmsg();
  ASSERT_TRUE(msg);

  msg->push_front(pb_serialized_request);
  msg->push_front(pb_serialized_header);
  msg->push_front(const_cast<char*>(EMPTY_FRAME));
  msg->push_front(client_identity_1);

  broker->database_snapshot("TEST_SERVICE_DISPATCH.START");
  broker->service_dispatch(service1, msg);
  broker->database_snapshot("TEST_SERVICE_DISPATCH.STOP");

  delete service1;
  delete msg;
  delete worker;
  LOG(INFO) << "TestProxy SERVICE_DISPATCH stop";
}


////////////////////////////////////////////////////////////////////////////////
TEST(TestProxy, BROKER_DELETE)
{
  delete broker;
}

TEST(TestProxy, BROKER_RUNTIME)
{
  int code;
  pthread_t broker;
  pthread_t worker;
  pthread_t client;

  LOG(INFO) << "TestProxy BROKER_RUNTIME start";
  /* Создать экземпляр Брокера */
  pthread_create (&broker, NULL, broker_task, NULL);
  usleep(100000);

  /* Создать одного Обработчика Службы NYSE */
  pthread_create (&worker, NULL, worker_task, NULL);
  usleep(100000);

  /* Создать одного клиента Службы NYSE */
  pthread_create (&client, NULL, client_task, NULL);

  /* Дождаться завершения работы Клиента */
  code = pthread_join (client, NULL);
  EXPECT_TRUE(code == 0);
  s_interrupted = 1;

  /* Дождаться завершения работы Обработчика */
  usleep(100000);
  code = pthread_join(worker, NULL);
  EXPECT_TRUE(code == 0);

  /* Дождаться завершения работы Брокера */
  usleep(100000);
  code = pthread_join(broker, NULL);
  EXPECT_TRUE(code == 0);
  LOG(INFO) << "TestProxy BROKER_RUNTIME stop";
}

////////////////////////////////////////////////////////////////////////////////
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
