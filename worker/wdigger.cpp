#include <assert.h>
#include "glog/logging.h"
#include "google/protobuf/stubs/common.h"

#include <vector>
#include <thread>
#include <memory>
#include <functional>

#include "xdb_rtap_application.hpp"
#include "xdb_rtap_environment.hpp"
#include "xdb_rtap_connection.hpp"

#include "wdigger.hpp"

#include "msg_common.h"
#include "mdp_worker_api.hpp"
#include "mdp_letter.hpp"
#include "mdp_zmsg.hpp"

using namespace mdp;

extern int interrupt_worker;

// Сокеты для обмена с DiggerWorker-ами через DiggerProxy
const char* endpoint_helper_frontend = "inproc://helper_frontend";
const char* endpoint_helper_backend  = "inproc://helper_backend";
const char* endpoint_helper_control  = "tcp://lo:5559";

const int DiggerProxy::kMaxThread = 3;

DiggerWorker::DiggerWorker(zmq::context_t &ctx, int sock_type, xdb::RtEnvironment* env) :
 m_context(ctx),
 m_worker(m_context, sock_type),
 m_interrupt(false),
 m_environment(env),
 m_db_connection(NULL)
{
  LOG(INFO) << "DiggerWorker start, copy context " << m_context;
}

DiggerWorker::~DiggerWorker()
{
  LOG(INFO) << "DiggerWorker stop, context " << m_context;
}

// Нитевой обработчик запросов
void DiggerWorker::work()
{
   int linger = 0;

   m_worker.connect(endpoint_helper_backend);
   m_worker.setsockopt (ZMQ_LINGER, &linger, sizeof (linger));
   LOG(INFO) << "DiggerWorker work() start connect to " << endpoint_helper_backend;

   try {
        while (!m_interrupt) {
            zmq::message_t identity;
            zmq::message_t msg;
            zmq::message_t copied_id;
            zmq::message_t copied_msg;
            m_worker.recv(&identity);
            m_worker.recv(&msg);
            LOG(INFO) << "DiggerWorker work() recv()";

            int replies = within(2);
            for (int reply = 0; reply < replies; ++reply) {
                s_sleep(within(2) + 1);
                copied_id.copy(&identity);
                copied_msg.copy(&msg);
                m_worker.send(copied_id, ZMQ_SNDMORE);
                m_worker.send(copied_msg);
                LOG(INFO) << "DiggerWorker work() send reply";
            }
        }
   }
   catch (std::exception &e) {
        LOG(ERROR) << e.what();
        m_interrupt = true;
        // Вызов исключения используется для прерывания работы DiggerWorker::work
        // путем удаления контекста из DiggerProxy.
   }

   try
   {
     m_worker.close();
   }
   catch(zmq::error_t error)
   {
     LOG(ERROR) << "DiggerWorker deleting socket: " << error.what();
   }

   LOG(INFO) << "DiggerWorker work() is done";
}


















DiggerProxy::DiggerProxy(xdb::RtEnvironment* env) :
 m_context(1),
 m_control(m_context, ZMQ_SUB),
 m_frontend(m_context, ZMQ_ROUTER),
 m_backend(m_context, ZMQ_DEALER),
 m_environment(env)
{
  LOG(INFO) << "DiggerProxy start, new context " << m_context;
}


DiggerProxy::~DiggerProxy()
{
  // NB: Ничего не делать, все ресурсы освобождаются при завершении run()
  LOG(INFO) << "DiggerProxy destructor";
}

// Отдельная нить бесконечной переадресации между Broker и BrokerWorker
void DiggerProxy::run()
{
    //
    std::vector<DiggerWorker *> m_worker;
    std::vector<std::thread *>  m_worker_thread;

    m_worker.clear();
    m_worker_thread.clear();

    try {
      // Выполняется в отдельной нити 
      m_frontend.bind(endpoint_helper_frontend);
      LOG(INFO) << "DiggerProxy binds to " << endpoint_helper_frontend;
      m_backend.bind(endpoint_helper_backend);
      LOG(INFO) << "DiggerProxy binds to " << endpoint_helper_backend;

      // Настройка управляющего сокета
      // Для получения точки подключения заменить в endpoint_helper_control
      // "lo" на "localhost"
      m_control.connect("tcp://localhost:5559");
      m_control.setsockopt(ZMQ_SUBSCRIBE, "", 0);

      // Создали пул Обработчиков Службы
      for (int i = 0; i < kMaxThread; ++i) {
        m_worker.push_back(new DiggerWorker(m_context, ZMQ_DEALER, m_environment));

        m_worker_thread.push_back(new std::thread(std::bind(&DiggerWorker::work, m_worker[i])));

        LOG(INFO) << "created DiggerWorker["<<i<<"] " << m_worker[i] << ", thread " << m_worker_thread[i];

        // m_worker_thread[i]->detach();
      }

      LOG(INFO) << "DiggerProxy (" << endpoint_helper_frontend
                << ", " << endpoint_helper_backend << ")";

      int rc = zmq_proxy_steerable (m_frontend, m_backend, nullptr, m_control);
      if (0 == rc)
      {
        LOG(INFO) << "DiggerProxy zmq::proxy finish";
      }
      else
      {
        throw error_t ();
      }

    LOG(INFO) << "DiggerProxy close frontend socket";
    m_frontend.close();
    LOG(INFO) << "DiggerProxy close backend socket";
    m_backend.close();
    LOG(INFO) << "DiggerProxy close control socket";
    m_control.close();

    sleep(1);

    LOG(INFO) << "DiggerProxy context closing";
    m_context.close();
    LOG(INFO) << "DiggerProxy context closed";
    // Завершение контекста Прокси должно вызвать сбой poll внутри DiggerWorker::work(),
    // что приведет к завершению треда и возможности join.

      LOG(INFO) << "DiggerProxy cleanup "<< m_worker.size()
                << " workers and threads, destroy context "
                << m_context;

      // Размер m_worker и m_worker_thread может быть меньше kMaxThread, 
      // если исключение возникло до занесения в них элементов. 
      for (int i = 0; i < kMaxThread; ++i) {

        LOG(INFO) << "delete DiggerWorker["<<i+1<<"/"<<m_worker.size()<<"] "
                  << m_worker[i] << ", thread " << m_worker_thread[i]
                  << "joinable: " << m_worker_thread[i]->joinable();
        m_worker_thread[i]->join();
        LOG(INFO) << "thread joined";

        delete m_worker[i];
        delete m_worker_thread[i];
      }
    }
    catch (std::exception &e)
    {
      LOG(ERROR) << "DiggerProxy catch the signal: " << e.what();
    }

}




















// 2 нити обработки запросов
Digger::Digger(std::string broker, std::string service, int verbose)
   :
   mdp::mdwrk(broker, service, verbose),
   m_helpers_frontend(m_context, ZMQ_ROUTER),
   m_helpers_control(m_context, ZMQ_PUB),
   m_digger_proxy(NULL),
   m_appli(NULL),
   m_environment(NULL),
   m_db_connection(NULL)
{
  m_appli = new xdb::RtApplication("DIGGER");
  m_appli->setOption("OF_CREATE",1);    // Создать если БД не было ранее
  m_appli->setOption("OF_LOAD_SNAP",1);
  m_appli->setOption("OF_RDWR",1);      // Открыть БД для чтения/записи
  m_appli->setOption("OF_DATABASE_SIZE",    1024 * 1024 * 1);
  m_appli->setOption("OF_MEMORYPAGE_SIZE",  1024);
  m_appli->setOption("OF_MAP_ADDRESS",      0x25000000);
  m_appli->setOption("OF_HTTP_PORT",        8083);

//3  m_appli->initialize();

//2  m_environment = m_appli->loadEnvironment("SINF");
//2  m_db_connection = m_environment->getConnection();
}

Digger::~Digger()
{
  LOG(INFO) << "Digger destructor";
  // RtEnvironment удаляется в деструкторе RtApplication
  delete m_appli;

  try
  {
    m_helpers_frontend.close();
    m_helpers_control.close();
  }
  catch(zmq::error_t error)
  {
    LOG(ERROR) << "Digger destructor: " << error.what();
  }
}

void Digger::run()
{
  interrupt_worker = false;
#if 1
  try
  {
    LOG(INFO) << "DiggerProxy creating";
    m_digger_proxy = new DiggerProxy(m_environment);

    LOG(INFO) << "DiggerProxy helpers connecting";
    //m_helpers_frontend.connect(endpoint_helper_frontend);

    LOG(INFO) << "DiggerProxy control connecting";
    m_helpers_control.bind(endpoint_helper_control);

    LOG(INFO) << "DiggerProxy starting thread";
    std::thread digger_proxy_thread(std::bind(&DiggerProxy::run, m_digger_proxy));

#endif
    // Ожидание завершения работы Прокси
    while (!interrupt_worker)
    {
      std::string *reply_to = new std::string;
      mdp::zmsg   *request  = NULL;

      LOG(INFO) << "Digger::recv() ready";
      request = recv (reply_to);
      if (request)
      {
        LOG(INFO) << "Digger::recv() got a message";
        handle_request (request, reply_to);
        delete request;
      }
      else
      {
        LOG(INFO) << "Digger::recv() got a NULL";
        interrupt_worker = true; // Worker has been interrupted
      }
      delete reply_to;
    }

    // Остановить DiggerProxy, послав служебное сообщение его управляющему сокету
    proxy_terminate();

    // Дождались останова
    LOG(INFO) << "Waiting joinable DiggerProxy thread";
    while (true)
    {
      if (digger_proxy_thread.joinable())
      {
        digger_proxy_thread.join();
        LOG(INFO) << "thread DiggerProxy joined";
        break;
      }
      else
      {
        sleep(1);
        LOG(INFO) << "tick join DiggerProxy waiting thread";
      }
    }

    // Освободить ресурсы DiggerProxy
    // NB: ресурсы выделялись в другой нити!
    delete m_digger_proxy;
  }
  catch(zmq::error_t error)
  {
    interrupt_worker = true;
    LOG(ERROR) << error.what();
  }
  catch(std::exception &e)
  {
    interrupt_worker = true;
    LOG(ERROR) << e.what();
  }
}

// Пауза прокси-треда
void Digger::proxy_pause()
{
  try
  {
    m_helpers_control.send("PAUSE", 5, 0);
  }
  catch(std::exception &e)
  {
    LOG(ERROR) << e.what();
  }
}

// Продолжить исполнение прокси-треда
void Digger::proxy_resume()
{
  try
  {
    m_helpers_control.send("RESUME", 6, 0);
  }
  catch(std::exception &e)
  {
    LOG(ERROR) << e.what();
  }
}

// Завершить работу прокси-треда
void Digger::proxy_terminate()
{
  try
  {
    LOG(INFO) << "Send TERMINATE to DiggerProxy";
    m_helpers_control.send("TERMINATE", 9, 0);
  }
  catch(std::exception &e)
  {
    LOG(ERROR) << e.what();
  }
}


int Digger::handle_request(mdp::zmsg* request, std::string*& reply_to)
{
  rtdbMsgType msgType;

  assert (request->parts () >= 2);
  LOG(INFO) << "Process new request with " << request->parts() 
            << " parts and reply to " << *reply_to;

  mdp::Letter *letter = new mdp::Letter(request);
  if (letter->GetVALIDITY())
  {
    msgType = letter->header().get_usr_msg_type();

    switch(msgType)
    {
      case SIG_D_MSG_READ_SINGLE:
      case SIG_D_MSG_READ_MULTI:
       handle_read(letter, reply_to);
       break;

      case SIG_D_MSG_WRITE_SINGLE:
      case SIG_D_MSG_WRITE_MULTI:
       handle_write(letter, reply_to);
       break;

      case ADG_D_MSG_ASKLIFE:
       handle_asklife(letter, reply_to);
       break;

      default:
       LOG(ERROR) << "Unsupported request type: " << msgType;
    }
  }
  else
  {
    LOG(ERROR) << "Readed letter "<<letter->header().get_exchange_id()<<" not valid";
  }

  delete letter;
  return 0;
}

int Digger::handle_read(mdp::Letter*, std::string*)
{
  LOG(INFO) << "Processing read request";
  return 0;
}

int Digger::handle_write(mdp::Letter*, std::string*)
{
  LOG(INFO) << "Processing write request";
  return 0;
}

int Digger::handle_asklife(mdp::Letter* letter, std::string* reply_to)
{
  RTDBM::AskLife *pb_asklife = NULL;
  mdp::zmsg      *response = new mdp::zmsg();

  response->push_front(const_cast<std::string&>(letter->SerializedData()));
  response->push_front(const_cast<std::string&>(letter->SerializedHeader()));
  response->wrap(reply_to->c_str(), "");

  pb_asklife = static_cast<RTDBM::AskLife*>(letter->data());
  std::cout << "asklife uid:"<< pb_asklife->user_exchange_id() << std::endl;
  LOG(INFO) << "Processing asklife from " << *reply_to
            << " uid:" << pb_asklife->user_exchange_id()
            << " sid:" << letter->header().get_exchange_id()
            << " dest:" << letter->header().get_proc_dest()
            << " origin:" << letter->header().get_proc_origin();

  send_to_broker((char*) MDPW_REPORT, NULL, response);
  delete response;

  return 0;
}

#if !defined _FUNCTIONAL_TEST
int main(int argc, char **argv)
{
  int  verbose = (argc > 1 && (0 == strcmp (argv [1], "-v")));
  char service_name[SERVICE_NAME_MAXLEN + 1];
  bool is_service_name_given = false;
  int  opt;
  Digger *engine = NULL;

  ::google::InitGoogleLogging(argv[0]);
  ::google::InstallFailureSignalHandler();

  while ((opt = getopt (argc, argv, "vs:")) != -1)
  {
     switch (opt)
     {
       case 'v':
         verbose = 1;
         break;

       case 's':
         strncpy(service_name, optarg, SERVICE_NAME_MAXLEN);
         service_name[SERVICE_NAME_MAXLEN] = '\0';
         is_service_name_given = true;
         break;

       case '?':
         if (optopt == 'n')
           fprintf (stderr, "Option -%c requires an argument.\n", optopt);
         else if (isprint (optopt))
           fprintf (stderr, "Unknown option `-%c'.\n", optopt);
         else
           fprintf (stderr,
                    "Unknown option character `\\x%x'.\n",
                    optopt);
         return 1;

       default:
         abort ();
     }
  }

  if (!is_service_name_given)
  {
    std::cout << "Service name not given.\nUse '-s <service>' option.\n";
    return(1);
  }

  try
  {
    engine = new Digger("tcp://localhost:5555", service_name, verbose);

    LOG(INFO) << "Hello Digger!";

    // Запуск нити DiggerProxy и нескольких DiggerWorker
    // ВЫполнение основного цикла работы приема-обработки-передачи сообщений
    engine->run();
  }
  catch(zmq::error_t err)
  {
    LOG(ERROR) << err.what();
  }
  delete engine;

  ::google::protobuf::ShutdownProtobufLibrary();
  ::google::ShutdownGoogleLogging();
  return 0;
}
#endif

