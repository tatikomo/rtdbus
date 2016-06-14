#include <pthread.h>
#include <syscall.h>
#include <string>
#include "gtest/gtest.h"
#include "glog/logging.h"
#include "google/protobuf/stubs/common.h"

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "helper.hpp"

#include "mdp_zmsg.hpp"
#include "mdp_common.h"
#include "mdp_broker.hpp"
#include "mdp_worker_api.hpp"
#include "mdp_client_async_api.hpp"
#include "mdp_proxy.hpp"

using namespace xdb;
#include "xdb_broker.hpp"
#include "xdb_impl_db_broker.hpp"
#include "xdb_rtap_application.hpp"
#include "xdb_rtap_environment.hpp"
#include "xdb_rtap_connection.hpp"

#include "wdigger.hpp"
#include "pulsar.hpp"

// Определения пользовательских сообщений и их сериализованных структур
#include "proto/common.pb.h"
#include "msg_common.h"
#include "msg_message.hpp"
#include "msg_adm.hpp"
#include "msg_sinf.hpp"

mdp::Digger *digger = NULL;
mdp::Broker *broker = NULL;
const std::string service_name = RTDB_NAME;
const std::string attributes_connection_to_broker = ENDPOINT_BROKER;
const char *BROKER_SNAP_FILE = (const char*) "broker_db.snap";

//  Раздельные признаки останова Брокера, Клиента, Обработчика
//  для возможности их совместного тестирования в одной программе,
//  но в разных нитях.
extern int interrupt_broker;
extern int interrupt_worker;
extern int interrupt_client;

bool  broker_ready_sign = false;
bool  worker_ready_sign = false;

// Описание одного элементарного теста, выполняемого сервисом client
// msg_type_send - тип посылаемого сообщения
// msg_type_recv - ожидаемый тип ответного сообщения
typedef struct {
  rtdbMsgType msg_type_send;
  rtdbMsgType msg_type_recv;
} one_test;

// Три теста - спросить состояние, ожидать ответа
one_test client_test_messages[] = {
  // спросить состояние, ожидать подтверждения
  { ADG_D_MSG_ASKLIFE,       ADG_D_MSG_EXECRESULT},
  // Отправить запрос на чтение данных, ожидать прочитанных данных 
  { SIG_D_MSG_READ_MULTI,    SIG_D_MSG_READ_MULTI},
  // Отправить запрос на модификацию данных, ожидать подтверждения
  { SIG_D_MSG_WRITE_MULTI,   ADG_D_MSG_EXECRESULT}
};

/*
 * Прототипы функций
 */
void  Dump(msg::Letter*);

/*
 * Реализация функций
 */
Pulsar::Pulsar(const std::string& broker, int verbose) 
  : mdcli(broker, verbose),
    m_channel(mdp::ChannelType::PERSISTENT)
{
  LOG(INFO) << "Create pulsar instance";
}

Pulsar::~Pulsar()
{
}


////////////////////////////////////////////////////////////////////////////////
void Dump(msg::Letter* letter)
{
  std::cout << (int)letter->header()->protocol_version();
  std::cout << "/" << letter->header()->exchange_id();
  std::cout << "/" << letter->header()->interest_id();
  std::cout << "/" << letter->header()->source_pid();
  std::cout << "/" << letter->header()->proc_dest();
  std::cout << "/" << letter->header()->proc_origin();
  std::cout << "/" << letter->header()->sys_msg_type();
  std::cout << "/" << letter->header()->usr_msg_type();
  std::cout << "/" << letter->header()->time_mark();
  std::cout << std::endl;
}

////////////////////////////////////////////////////////////////////////////////
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
 * Используемая служба: RTDB (service_name)
 */
static void *
client_task (void* /*args*/)
{
  int        verbose   = 1;
  int        iteration = 0;
  mdp::zmsg *request   = NULL;
  mdp::zmsg *report    = NULL;
  Pulsar    *client    = new Pulsar (attributes_connection_to_broker, verbose);
  int        count;
  std::string       pb_serialized_request;
  msg::AskLife     *ask_life;
  msg::ReadMulti   *read_multi;
  char service_endpoint[ENDPOINT_MAXLEN + 1];
  static int        user_exchange_id = 0;
  int               service_status;
  int               status;
  msg::MessageFactory *message_factory = NULL;
  char str_thread_id[ENDPOINT_MAXLEN + 1];
  int               sid; // LWP id
  std::string       tag;
  bool              wait_response = true;

  LOG(INFO) << "Start client thread";
  try
  {
    LOG(INFO) << "Ask endpoint for service " << service_name;
    service_status = client->ask_service_info(service_name, service_endpoint, ENDPOINT_MAXLEN);
    LOG(INFO) << "Get endpoint to '" << service_endpoint << "' for " << service_name;

    switch(service_status)
    {
      case 200:
      LOG(INFO) << service_name << " status OK : " << service_status;
      break;

      case 400:
      LOG(INFO) << service_name << " status BAD_REQUEST : " << service_status;
      break;

      case 404:
      LOG(INFO) << service_name << " status NOT_FOUND : " << service_status;
      break;

      case 501:
      LOG(INFO) << service_name << " status NOT_SUPPORTED : " << service_status;
      break;

      default:
      LOG(INFO) << service_name << " status UNKNOWN : " << service_status;
    }

    if (200 != service_status)
    {
      LOG(INFO) << service_name << " status not OK : " << service_status;
      delete client;
      pthread_exit(NULL);
      return NULL;
    }

    sid = syscall(SYS_gettid);
    sprintf(str_thread_id, "CLIENT_%d", sid);

    message_factory = new msg::MessageFactory(str_thread_id);

    for (iteration = 0; iteration < 2; iteration++)
    {
        switch(client_test_messages[iteration].msg_type_send)
        {
          case ADG_D_MSG_ASKLIFE:
            // ===============================================================================
            // Проверка прохождения сообщения типа ADG_D_MSG_ASKLIFE
            // ===============================================================================
            ask_life = static_cast<msg::AskLife*>(message_factory->create(client_test_messages[iteration].msg_type_send));
            ask_life->set_destination(service_name);

            //  Send some ADG_D_MSG_ASKLIFE orders
            request = new mdp::zmsg ();

            ask_life->set_exec_result(10);
            request->push_front(const_cast<std::string&>(ask_life->data()->get_serialized()));
            request->push_front(const_cast<std::string&>(ask_life->header()->get_serialized()));

            LOG(INFO) << "CLIENT: Send message type:" << client_test_messages[iteration].msg_type_send
                      << " id:"<<user_exchange_id++;
            client->send (service_name, request);
            wait_response = true;

            delete request;
            delete ask_life;
          break;

          case SIG_D_MSG_READ_MULTI:
            // ===============================================================================
            // Проверка прохождения сообщения типа SIG_D_MSG_READ_MULTI
            // ===============================================================================
            read_multi = static_cast<msg::ReadMulti*>(message_factory->create(client_test_messages[iteration].msg_type_send));
            read_multi->set_destination(service_name);

            request = new mdp::zmsg ();
            tag = "/INVT1/TIME_AVAILABLE.DATEHOURM";
            read_multi->add(tag, xdb::DB_TYPE_UNDEF, NULL);
            tag = "/INVT1/TIME_AVAILABLE.SHORTLABEL";
            read_multi->add(tag, xdb::DB_TYPE_UNDEF, NULL);

            request->push_front(const_cast<std::string&>(read_multi->data()->get_serialized()));
            request->push_front(const_cast<std::string&>(read_multi->header()->get_serialized()));

            LOG(INFO) << "CLIENT: Send message type:" << client_test_messages[iteration].msg_type_send
                      << " id:"<<user_exchange_id++;

            service_status = client->ask_service_info(service_name, service_endpoint, ENDPOINT_MAXLEN);
            // Брокер ответил - Служба известна
            if (service_status == 200)
            {
              LOG(INFO) << "Endpoint for \"" << service_name << "\" is " << service_endpoint;
            
              client->send (service_name, request, mdp::ChannelType::DIRECT);
              wait_response = true;
            }
            else
            {
              LOG(ERROR) << "Broker response (" << service_status << ") about service \""
                         << service_name << "\" is not equal to 200";
              wait_response = false;
            }

            delete request;
            delete read_multi;
          break;

          default:
            LOG(INFO) << "Unsupported resuest type: " << client_test_messages[iteration].msg_type_send;
            wait_response = false;
            // продолжить проверку в соответствии с заданным массивом client_test_messages
            continue;
        }

        count = 0;

        //  Wait for all trading reports
        while (wait_response) {
            status = client->recv (report);
            if (Pulsar::RECEIVE_OK != status)
                break;
            ++count;
            LOG(INFO) << "Receive message id="<<count<<" from worker";
            report->dump();
            assert (report->parts () >= 2);
            
#if 0
            mdp_letter = new mdp::Letter(report);
    //        TODO: Доделать разбор сообщений
            Dump(mdp_letter);

            delete mdp_letter;
#else
#warning    "Дамп сообщений заблокирован"
#endif
            delete report;
        }
        // Приняли ровно столько, сколько ранее отправили
        //EXPECT_TRUE(count == 1/*user_exchange_id*/);
    } // end for
  }
  catch (zmq::error_t err)
  {
      std::cout << "E: " << err.what() << std::endl;
  }

  // Завершить исполнение Брокера - все сообщения отправлены/получены
  interrupt_broker = 1;
  LOG(INFO) << "Stop client thread";
  delete message_factory;
  delete client;
  pthread_exit(NULL);
  return NULL; /* NOTREACHED */ 
}

/*
 * Рабочий цикл программы-Обработчика Digger для службы NYSE
 * Тестируется класс mdwrk
 * Создается БД DIGGER:RTAP
 */
static void *
worker_task (void* /*args*/)
{
  LOG(INFO) << "Start worker thread";
  worker_ready_sign = false;

  try
  {
    mdp::Digger *engine = new mdp::Digger(attributes_connection_to_broker,
                                service_name);

    // Обозначить завершение своей инициализации
    worker_ready_sign = true;
    engine->run();
    delete engine;
  }
  catch(zmq::error_t err)
  {
    std::cout << "E: " << err.what() << std::endl;
  }

  LOG(INFO) << "Stop worker thread";
  pthread_exit(NULL);
  return NULL; /* NOTREACHED */ 
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
  broker_ready_sign = false;
  usleep(100000);

  try
  {
     s_version_assert (3, 2);
     s_catch_signals ();
     broker = new mdp::Broker(verbose);

     // Обозначить завершение своей инициализации
     broker_ready_sign = true;

     broker->start_brokering();
  }
  catch (zmq::error_t err)
  {
     std::cout << "E: " << err.what() << std::endl;
  }

  if (interrupt_broker)
  {
     LOG(WARNING)<<"Interrupt received, shutting down...";
  }

  LOG(INFO) << "Stop broker thread";
  delete broker;

  pthread_exit(NULL);
  return NULL; /* NOTREACHED */ 
}

////////////////////////////////////////////////////////////////////////////////
TEST(TestProxy, BROKER_RUNTIME)
{
  int code;
  pthread_t broker;
  pthread_t worker;
  pthread_t client;
  time_t    broker_ready_time;
  time_t    worker_ready_time;
  time_t    time_now;

  unlink(BROKER_SNAP_FILE);

  LOG(INFO) << "TestProxy BROKER_RUNTIME start";

  /* Создать экземпляр Брокера */
  pthread_create (&broker, NULL, broker_task, NULL);

  // Прождать три секунды до инициализации Брокера
  time_now = time(0);
  broker_ready_sign = false;
  broker_ready_time = time_now + 2; // Таймаут инициализации 2 секунды

  fputs("\nStarting broker: ", stdout);
  while ((time_now < broker_ready_time) && (!broker_ready_sign))
  {
   usleep(100000);
   putchar('.');
   time_now = time(0);
  }
  puts("");

  if (!broker_ready_sign)
  {
    LOG(ERROR) << "Broker doesn't get ready for a 3 seconds, exiting";
    exit(1);
  }

  /* Создать одного Обработчика Службы NYSE */
  pthread_create (&worker, NULL, worker_task, NULL);
  time_now = time(0);
  worker_ready_sign = false;
  worker_ready_time = time_now + 2; // Таймаут инициализации 2 секунды
  fputs("\nStarting worker: ", stdout);
  while ((time_now < worker_ready_time) && (!worker_ready_sign))
  {
   usleep(500000);
   putchar('.');
   time_now = time(0);
  }
  puts("");

  /* Создать одного клиента Службы NYSE */
  pthread_create (&client, NULL, client_task, NULL);

  /* Дождаться завершения работы Клиента */
  code = pthread_join (client, NULL);
  EXPECT_TRUE(code == 0);

  // Остановить Обработчика
  interrupt_worker = 1;
  /* Дождаться завершения работы Обработчика */
  usleep(1000000);
  code = pthread_join(worker, NULL);
  EXPECT_TRUE(code == 0);

  // Остановить Брокера
  interrupt_broker = 1;
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
