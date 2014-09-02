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

using namespace xdb;
#include "wdigger.hpp"
#include "pulsar.hpp"

// Определения пользовательских сообщений и их сериализованных структур
#include "msg_common.h"
#include "proto/common.pb.h"

mdp::Digger *digger = NULL;
mdp::Broker *broker = NULL;
std::string service_name = "SINF";
const char *attributes_connection_to_broker = (const char*) "tcp://localhost:5555";
const char *BROKER_SNAP_FILE = (const char*) "broker_db.snap";
const char *RTAP_SNAP_FILE = (const char*) "SINF.snap";

/*
 * Прототипы функций
 */
void         Dump(mdp::Letter*);

/*
 * Реализация функций
 */
Pulsar::Pulsar(std::string broker, int verbose) : mdcli(broker, verbose)
{
  LOG(INFO) << "Create pulsar instance";
}

////////////////////////////////////////////////////////////////////////////////
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
 * Используемая служба: SINF (service_name)
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
  const int         i_num_sent_letters = 1;

  LOG(INFO) << "Start client thread";
  try
  {
    mdp_letter = new mdp::Letter(ADG_D_MSG_ASKLIFE, dest);

    //  Send some ADG_D_MSG_ASKLIFE orders
    for (user_exchange_id = 0; user_exchange_id < i_num_sent_letters; user_exchange_id++) {
        request = new mdp::zmsg ();

        // Единственное поле, которое может устанавливать клиент
        // напрямую в ADG_D_MSG_ASKLIFE
        static_cast<RTDBM::AskLife*>(mdp_letter->mutable_data())->set_user_exchange_id(user_exchange_id);
        request->push_front(const_cast<std::string&>(mdp_letter->SerializedData()));
        request->push_front(const_cast<std::string&>(mdp_letter->SerializedHeader()));
        LOG(INFO) << "Send message id="<<user_exchange_id<<" from client";
        client->send (service_name.c_str(), request);
        delete request;
    }
    delete mdp_letter;

    EXPECT_EQ(user_exchange_id, i_num_sent_letters);
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
 * Создается БД DIGGER:RTAP
 */
static void *
worker_task (void* /*args*/)
{
  int verbose = 1;

  LOG(INFO) << "Start worker thread";
  try
  {
    mdp::Digger *engine = new mdp::Digger(attributes_connection_to_broker,
                                service_name.c_str(),
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
TEST(TestProxy, BROKER_RUNTIME)
{
  int code;
  pthread_t broker;
  pthread_t worker;
  pthread_t client;

  unlink(BROKER_SNAP_FILE);
  unlink(RTAP_SNAP_FILE);

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
