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
#include "pulsar.hpp"

// Определения пользовательских сообщений и их сериализованных структур
#include "msg_common.h"
#include "proto/common.pb.h"

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

const char *attributes_connection_to_broker = (const char*)"tcp://localhost:5555";
const char *BROKER_SNAP_FILE = (const char*)"broker_db.snap";
xdb::Service *service1 = NULL;
xdb::Service *service2 = NULL;
xdb::Worker  *worker = NULL;
int64_t service1_id;
int64_t service2_id;

typedef enum
{
  F_ALL,          // удалить всех Обработчиков
  F_EXPIRED,      // Только тех, кто действительно проссрочен
  F_OCCUPIED      // Только тех, кто обрабатывает Сообщения
} PurgeFilter_t;

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
        static_cast<RTDBM::ExecResult*>(pb_message)->set_exec_result(user_exchange % 10);
        static_cast<RTDBM::ExecResult*>(pb_message)->set_failure_cause(user_exchange % 100);
        break;

    case ADG_D_MSG_ASKLIFE:
        pb_message = new RTDBM::AskLife;
        static_cast<RTDBM::AskLife*>(pb_message)->set_status(user_exchange % 10);
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

void purge_workers(PurgeFilter_t filter)
{
  xdb::ServiceList  *sl = broker->get_internal_db_api()->GetServiceList();
  xdb::Service      *service = sl->first();
  xdb::Worker       *wrk = NULL;

  broker->database_snapshot("PURGE_WORKERS.START");
  while(NULL != service)
  {
    /* Пройти по списку Сервисов */
    /*while (NULL != (*/wrk = broker->get_internal_db_api()->PopWorker(service);/*))*/
    if (wrk)
    {
        LOG(INFO) <<service->GetID()<<":"<<service->GetNAME()<<" => "<<wrk->GetIDENTITY();
        // Удалить только просроченные записи
        // Или все записи, если фильтр позволяет
        if ((filter == F_ALL) || (wrk->Expired ()))
        {
          LOG(INFO) << "Deleting expired worker: " << wrk->GetIDENTITY();
          broker->release (wrk, 0); // Обработчик не подавал признаков жизни
        }
        else if (filter == F_OCCUPIED)
        {
          // TODO: Удалить тех Обработчиков, за которыми закреплены Сообщения
        }

        delete wrk; // Обработчик жив, просто освободить память
    }
    service = sl->next();
  }
  broker->database_snapshot("PURGE_WORKERS.STOP");
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
  std::cout << "/" << letter->header().get_interest_id();
  std::cout << "/" << letter->header().get_source_pid();
  std::cout << "/" << letter->header().get_proc_dest();
  std::cout << "/" << letter->header().get_proc_origin();
  std::cout << "/" << letter->header().get_sys_msg_type();
  std::cout << "/" << letter->header().get_usr_msg_type();
  std::cout << "[" << kokoko->exec_result();
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

  pb_exec_result_request.set_exec_result(1234);
  pb_exec_result_request.set_failure_cause(567);
  pb_exec_result_request.SerializeToString(&pb_serialized_request);  

  letter1 = new mdp::Letter(ADG_D_MSG_EXECRESULT, "NYSE", &pb_serialized_request);
  Dump(letter1);

  letter2 = new mdp::Letter(ADG_D_MSG_EXECRESULT, "NYSE");
  RTDBM::ExecResult* exec_result = static_cast<RTDBM::ExecResult*>(letter2->mutable_data());
  exec_result->set_exec_result(104);
  exec_result->set_failure_cause(105);
  Dump(letter2);

  pb_exec_result_request.ParseFromString(letter2->SerializedData());
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

  unlink(BROKER_SNAP_FILE);

  LOG(INFO) << "TestProxy BROKER_INTERNAL start";
  broker = new mdp::Broker(true); // be verbose
  ASSERT_TRUE (broker != NULL);

  // 1. Создать БД, попытаться открыть снимок
  // 2. Связать сокет со службой Брокера (bind)
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
  pb_header.set_interest_id(2);
  pb_header.set_source_pid(getpid());
  pb_header.set_proc_dest("dest");
  pb_header.set_proc_origin("src");
  pb_header.set_sys_msg_type(USER_MESSAGE_TYPE);
  pb_header.set_usr_msg_type(ADG_D_MSG_EXECRESULT);

  pb_exec_result_request.set_exec_result(3);
  pb_exec_result_request.set_failure_cause(4);

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
  purge_workers(F_EXPIRED);
  // На выходе не должно быть удаленных Обработчиков

  // Установить срок годности Обработчика wrk "в прошлое"
  // и повторить проверку. На этот раз один обработчик д.б. удален
  wrk->SetEXPIRATION(mark);
  delete wrk;
  purge_workers(F_EXPIRED);
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
  pb_header.set_exchange_id(1234567);
  pb_header.set_interest_id(7654321);
  pb_header.set_source_pid(9999);
  pb_header.set_proc_dest("В чащах юга жил-был Цитрус?");
  pb_header.set_proc_origin("Да! Но фальшивый экземпляр.");
  pb_header.set_sys_msg_type(USER_MESSAGE_TYPE);
  pb_header.set_usr_msg_type(ADG_D_MSG_EXECRESULT);

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
  // Очистить временные данные из БД перед сохранением её снимка
  // -----------------------------------------------------------

#ifdef USE_EXTREMEDB_HTTP_SERVER
  printf("Visit 'http://localhost:8082'\nPress any key to exit\n");
  int input = getchar();
#endif
  delete broker;
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
