#include <assert.h>
#include "glog/logging.h"
#include "google/protobuf/stubs/common.h"

#if defined HAVE_CONFIG_H
#include "config.h"
#endif

#include <vector>
#include <thread>
#include <memory>
#include <functional>

#include "xdb_rtap_application.hpp"
#include "xdb_rtap_environment.hpp"
#include "xdb_rtap_connection.hpp"
#include "xdb_rtap_point.hpp"

#include "wdigger.hpp"

#include "msg_common.h"
#include "mdp_worker_api.hpp"
#include "mdp_zmsg.hpp"

// общий интерфейс сообщений
#include "msg_message.hpp"
// сообщения общего порядка
#include "msg_adm.hpp"
// сообщения по работе с БДРВ
#include "msg_sinf.hpp"

using namespace mdp;

extern int interrupt_worker;

const int DiggerProxy::kMaxThread = 15;
const int Digger::DatabaseSizeBytes = 1024 * 1024 * DIGGER_DB_SIZE_MB;

// --------------------------------------------------------------------------------
DiggerWorker::DiggerWorker(zmq::context_t &ctx, int sock_type, xdb::RtEnvironment* env) :
 m_context(ctx),
 m_worker(m_context, sock_type),
 m_interrupt(false),
 m_environment(env),
 m_db_connection(NULL),
 m_message_factory(NULL)
{
  LOG(INFO) << "DiggerWorker created";
}

// --------------------------------------------------------------------------------
DiggerWorker::~DiggerWorker()
{
  LOG(INFO) << "DiggerWorker destroyed";
}

// Нитевой обработчик запросов
// Выход из функции:
// 1. по исключению
// 2. получение строки идентификации, равной "TERMINATE"
// --------------------------------------------------------------------------------
void DiggerWorker::work()
{
   int linger = 0;
   zmsg *request = NULL;
   zmsg *replay = NULL;
   xdb::RtPoint *point = NULL;

   m_worker.connect(ENDPOINT_SINF_BACKEND);
   m_worker.setsockopt (ZMQ_LINGER, &linger, sizeof (linger));
   LOG(INFO) << "DiggerWorker thread connects to " << ENDPOINT_SINF_BACKEND;

   try {
        m_message_factory = new msg::MessageFactory(DIGGER_NAME);

        // Каждая нить процесса, желающая работать с БДРВ,
        // должна получить свой экземпляр RtConnection
        m_db_connection = m_environment->getConnection();

        //
        //
        //
        //
        while (!m_interrupt)
        {
          request = new zmsg (m_worker);
          assert (request);

          LOG(INFO) << "received request:";
          request->dump ();

          // replay address
          std::string identity  = request->pop_front();
          if ((identity.size() == 9) && (identity.compare("TERMINATE") == 0))
          {
              LOG(INFO) << "Got TERMINATE command, shuttingdown this DiggerWorker thread";
              m_interrupt = true;
              // Перед выходом очистить уже прочитанный запрос и identity
              delete request;
              break;
          }

          if ((identity.size() == 5) && (identity.compare("PROBE") == 0))
          {
              LOG(INFO) << "TODO: Got PROBE command, send info about this DiggerWorker thread";
              m_interrupt = false;
              delete request;
              break;
          }

          std::string empty     = request->pop_front();
//1          std::string header    = request->pop_front();
//1          std::string payload   = request->pop_front();

#if 0
          // Для тестирования - ищем определенную точку в БДРВ
#warning "2015/03/13 Продолжить здесь"
          //
          //
          point = m_db_connection->locate("/KA4003/K20003/3/PT01d");
          if (!point)
            LOG(WARNING) << "Not found";
          delete point;
          //
          //
          //
          replay = new zmsg ();
          replay->push_front (const_cast<char*>(payload.c_str()));
          replay->push_front (const_cast<char*>(header.c_str()));
          replay->wrap (const_cast<char*>(identity.c_str()), EMPTY_FRAME);
          LOG(INFO) << "send replay:";
          replay->dump ();
          replay->send (m_worker);
#else
          processing(request, identity);
#endif

          delete request;
          //1 delete replay;
        }

        m_worker.close();
   }
   catch (std::exception &e) {
     LOG(ERROR) << e.what();
     m_interrupt = true;
   }

   delete m_db_connection;
   delete m_message_factory;

   LOG(INFO) << "DiggerWorker thread is done";
}


// --------------------------------------------------------------------------------
// Функция первично обрабатывает полученный запрос на содержимое БДРВ.
// NB: Запросы к БД обрабатываются конкурентно, в фоне, нитями DiggerWorker-ов.
int DiggerWorker::processing(mdp::zmsg* request, std::string &identity)
{
  rtdbMsgType msgType;

  LOG(INFO) << "Process new request with " << request->parts() 
            << " parts and reply to " << identity;

  msg::Letter *letter = m_message_factory->create(request);
  if (letter->valid())
  {
    msgType = letter->header()->usr_msg_type();

    switch(msgType)
    {
      case SIG_D_MSG_READ_MULTI:
       handle_read(letter, identity);
      break;

      case SIG_D_MSG_WRITE_MULTI:
       handle_write(letter, identity);
      break;

      default:
       LOG(ERROR) << "Unsupported request type: " << msgType;
    }
  }
  else
  {
    LOG(ERROR) << "Readed letter "<<letter->header()->exchange_id()<<" not valid";
  }

  delete letter;
  return 0;
}

// --------------------------------------------------------------------------------
int DiggerWorker::handle_read(msg::Letter* letter, std::string& identity)
{
  msg::ReadMulti *read_msg = dynamic_cast<msg::ReadMulti*>(letter);
  xdb::RtPoint   *point = NULL;
  mdp::zmsg      *response = new mdp::zmsg();

#if 0
  std::cout << "Processing ReadMulti from " << identity
            << " sid:" << read_msg->header()->exchange_id()
            << " iid:" << read_msg->header()->interest_id()
            << " dest:" << read_msg->header()->proc_dest()
            << " origin:" << read_msg->header()->proc_origin() << std::endl;
#endif

  LOG(INFO) << "Processing ReadMulti from " << identity
            << " sid:" << read_msg->header()->exchange_id()
            << " iid:" << read_msg->header()->interest_id()
            << " dest:" << read_msg->header()->proc_dest()
            << " origin:" << read_msg->header()->proc_origin();

  for (int idx = 0; idx < read_msg->num_items(); idx++)
  {
    const msg::Value& todo = read_msg->item(idx);

    point = m_db_connection->locate(todo.tag().c_str());

    if (!point) {
      // Нет такой точки в БДРВ
      LOG(ERROR) << "Request unexistent point \"" << todo.tag() << "\"";
    }
    delete point;
  }





  read_msg->set_destination(identity);
  response->push_front(const_cast<std::string&>(read_msg->data()->get_serialized()));
  response->push_front(const_cast<std::string&>(read_msg->header()->get_serialized()));
  response->wrap(identity.c_str(), EMPTY_FRAME);

  response->send (m_worker); //1

  delete response;
  return 0;
}

// --------------------------------------------------------------------------------
int DiggerWorker::handle_write(msg::Letter* letter, std::string& identity)
{
  msg::WriteMulti *write_msg = dynamic_cast<msg::WriteMulti*>(letter);
  LOG(INFO) << "Processing WriteMulti request to " << identity;

  send_exec_result(0, identity);
  return 0;
}

// отправить адресату сообщение о статусе исполнения команды
// --------------------------------------------------------------------------------
void DiggerWorker::send_exec_result(int exec_val, std::string& identity)
{
  std::string OK = "хорошо";
  std::string FAIL = "плохо";
  msg::ExecResult *msg_exec_result = 
        dynamic_cast<msg::ExecResult*>(m_message_factory->create(ADG_D_MSG_EXECRESULT));
  mdp::zmsg       *response = new mdp::zmsg();

  msg_exec_result->set_destination(identity);
  msg_exec_result->set_exec_result(exec_val);
  msg_exec_result->set_failure_cause(0, OK);

  response->push_front(const_cast<std::string&>(msg_exec_result->data()->get_serialized()));
  response->push_front(const_cast<std::string&>(msg_exec_result->header()->get_serialized()));
  response->wrap(identity.c_str(), "");

/*  std::cout << "Send ExecResult to " << identity
            << " with status:" << msg_exec_result->exec_result()
            << " sid:" << msg_exec_result->header()->exchange_id()
            << " iid:" << msg_exec_result->header()->interest_id()
            << " dest:" << msg_exec_result->header()->proc_dest()
            << " origin:" << msg_exec_result->header()->proc_origin() << std::endl;*/

  LOG(INFO) << "Send ExecResult to " << identity
            << " with status:" << msg_exec_result->exec_result()
            << " sid:" << msg_exec_result->header()->exchange_id()
            << " iid:" << msg_exec_result->header()->interest_id()
            << " dest:" << msg_exec_result->header()->proc_dest()
            << " origin:" << msg_exec_result->header()->proc_origin();

#warning "replace send_to_broker(msg::zmsg*) to send(msg::Letter*)"
  // TODO: Для упрощения обработки сообщения не следует создавать zmsg и заполнять его
  // данными из Letter, а сразу напрямую отправлять Letter потребителю.
  // Возможно, следует указать тип передачи - прямой или через брокер.
  //1 send_to_broker((char*) MDPW_REPORT, NULL, response);
  response->send (m_worker); //1

  delete response;
}


// --------------------------------------------------------------------------------
DiggerProxy::DiggerProxy(zmq::context_t &ctx, xdb::RtEnvironment* env) :
 m_context(ctx),
 m_control(m_context, ZMQ_SUB),
 m_frontend(m_context, ZMQ_ROUTER),
 m_backend(m_context, ZMQ_DEALER),
 m_environment(env)
{
  LOG(INFO) << "DiggerProxy start, new context " << m_context;
}

// --------------------------------------------------------------------------------
//  Класс-прокси запросов от главной нити (Digger) к исполнителям (DiggerWorker)
// --------------------------------------------------------------------------------
DiggerProxy::~DiggerProxy()
{
  // NB: Ничего не делать, все ресурсы освобождаются при завершении run()
  LOG(INFO) << "DiggerProxy destructor";
}

// --------------------------------------------------------------------------------
// Отдельная нить бесконечной переадресации между нитями Digger и DiggerWorker
//  Условия завершения работы:
//  1. Исключение zmq::error
//  2. Получение от нити Digger сообщения "TERMINATE", что завершает работу
//  функции zmq_proxy_steerable
// --------------------------------------------------------------------------------
void DiggerProxy::run()
{
    int mandatory = 1;
    // Список экземпляров класса DiggerWorker
    std::vector<DiggerWorker *> m_worker;
    // Список экземпляров нитей DiggerWorker::work()
    std::vector<std::thread *>  m_worker_thread;

    try {
      // Сокет прямого подключения к экземплярям DiggerWorker
      // NB: Выполняется в отдельной нити 
      m_frontend.setsockopt (ZMQ_ROUTER_MANDATORY, &mandatory, sizeof (mandatory));
      m_frontend.bind("tcp://lo:5556" /*ENDPOINT_SINF_FRONTEND*/);
      LOG(INFO) << "DiggerProxy binds to DiggerWorkers frontend " << ENDPOINT_SINF_FRONTEND;
      m_backend.bind(ENDPOINT_SINF_BACKEND);
      LOG(INFO) << "DiggerProxy binds to DiggerWorkers backend " << ENDPOINT_SINF_BACKEND;

      // Настройка управляющего сокета
      LOG(INFO) << "DiggerProxy is ready to connect with DiggerWorkers control";
      m_control.connect(ENDPOINT_SINF_PROXY_CTRL);
      LOG(INFO) << "DiggerProxy connects to DiggerWorkers control " << ENDPOINT_SINF_PROXY_CTRL;
      m_control.setsockopt(ZMQ_SUBSCRIBE, "", 0);

      // Создали пул Обработчиков Службы
      for (int i = 0; i < kMaxThread; ++i) {
        m_worker.push_back(new DiggerWorker(m_context, ZMQ_DEALER, m_environment));

        m_worker_thread.push_back(new std::thread(std::bind(&DiggerWorker::work, m_worker[i])));

        LOG(INFO) << "created DiggerWorker["<<i<<"] " << m_worker[i] << ", thread " << m_worker_thread[i];
      }

      LOG(INFO) << "DiggerProxy (" << ENDPOINT_SINF_FRONTEND
                << ", " << ENDPOINT_SINF_BACKEND << ")";

      int rc = zmq_proxy_steerable (m_frontend, m_backend, nullptr, m_control);
      if (0 == rc)
      {
        LOG(INFO) << "DiggerProxy zmq::proxy finish successfuly";
      }
      else
      {
        LOG(ERROR) << "DiggerProxy zmq::proxy failure, rc=" << rc;
        //throw error_t ();
      }

      // Вызвать останов функции DiggerWorker::work() для завершения треда
      for (int i = 0; i < kMaxThread; ++i)
      {
        LOG(INFO) << "Send TERMINATE to DiggerWorker " << i;
        m_backend.send("TERMINATE", 9, 0);
      }

      m_frontend.close();
      m_backend.close();
      m_control.close();

      LOG(INFO) << "DiggerProxy cleanup "<<m_worker.size()<<" workers and threads";

      for (int i = 0; i < kMaxThread; ++i)
      {
        LOG(INFO) << "delete DiggerWorker["<<i+1<<"/"<<m_worker.size()<<"] "
                  << m_worker[i]
                  << ", thread " << m_worker_thread[i]
                  << ", joinable: " << m_worker_thread[i]->joinable();
        m_worker_thread[i]->join();
        LOG(INFO) << "DiggerProxy joins DiggerWorker' tread " << i+1;

        delete m_worker[i];
        delete m_worker_thread[i];
      }
    }
    catch(zmq::error_t error)
    {
      LOG(ERROR) << "DiggerProxy catch: " << error.what();
    }
    catch (std::exception &e)
    {
      LOG(ERROR) << "DiggerProxy catch the signal: " << e.what();
    }
}


// Класс-расширение штатной Службы для обработки запросов к БДРВ
// --------------------------------------------------------------------------------
Digger::Digger(std::string broker_endpoint, std::string service, int verbose)
   :
   mdp::mdwrk(broker_endpoint, service, verbose),
   m_helpers_control(m_context, ZMQ_PUB),
   m_digger_proxy(NULL),
   m_proxy_thread(NULL),
   m_message_factory(new msg::MessageFactory(DIGGER_NAME)),
   m_appli(NULL),
   m_environment(NULL),
   m_db_connection(NULL)
{
  m_appli = new xdb::RtApplication("DIGGER");
  m_appli->setOption("OF_CREATE",   1);    // Создать если БД не было ранее
  m_appli->setOption("OF_LOAD_SNAP",1);
  m_appli->setOption("OF_SAVE_SNAP",1);
  // Максимальное число подключений к БД:
  // a) по одному на каждый экземпляр DiggerWorker
  // b) один для Digger
  // c) один для мониторинга
  m_appli->setOption("OF_MAX_CONNECTIONS",  DiggerProxy::kMaxThread + 4);
  m_appli->setOption("OF_RDWR",1);      // Открыть БД для чтения/записи
  m_appli->setOption("OF_DATABASE_SIZE",    Digger::DatabaseSizeBytes);
  m_appli->setOption("OF_MEMORYPAGE_SIZE",  1024);
  m_appli->setOption("OF_MAP_ADDRESS",      0x20000000);
#if defined USE_EXTREMEDB_HTTP_SERVER
  m_appli->setOption("OF_HTTP_PORT",        8083);
#endif
  m_appli->setOption("OF_DISK_CACHE_SIZE",  0);

  m_appli->initialize();

  m_environment = m_appli->loadEnvironment("SINF");
  // Каждая нить процесса, желающая работать с БДРВ, должна получить свой экземпляр
  m_db_connection = m_environment->getConnection();
}

// --------------------------------------------------------------------------------
Digger::~Digger()
{
  LOG(INFO) << "Digger destructor";

  delete m_message_factory;

  delete m_db_connection;

  // RtEnvironment удаляется в деструкторе RtApplication
  delete m_appli;

  try
  {
    m_helpers_control.close();

    delete m_proxy_thread;
  }
  catch(zmq::error_t error)
  {
    LOG(ERROR) << "Digger destructor: " << error.what();
  }
}

// Запуск DiggerProxy и цикла получения сообщений
// --------------------------------------------------------------------------------
void Digger::run()
{
  interrupt_worker = false;

  try
  {
    LOG(INFO) << "DiggerProxy creating";
    m_digger_proxy = new DiggerProxy(m_context, m_environment);

    LOG(INFO) << "DiggerProxy starting thread";
    m_proxy_thread = new std::thread(std::bind(&DiggerProxy::run, m_digger_proxy));
    sleep(2);

    LOG(INFO) << "DiggerProxy control connecting";
    m_helpers_control.bind("tcp://lo:5557" /*ENDPOINT_SINF_PROXY_CTRL*/);

    // Ожидание завершения работы Прокси
    while (!interrupt_worker)
    {
      std::string *reply_to = new std::string;
      mdp::zmsg   *request  = NULL;

      LOG(INFO) << "Digger::recv() ready";
      // NB: Функция recv возвращает только PERSISTENT-сообщения,
      // DIRECT-сообщения передаются в DiggerProxy, а тот пересылает их DiggerWorker-ам
      // Чтение DIRECT-сокета заблокировано в zmq::poll функции mdwrk::recv()
      request = recv (reply_to);
      if (request)
      {
        LOG(INFO) << "Digger::recv() got a message";

        // NB: попробовать передать сообщения от Брокера сразу в очередь DiggerWorker-ам
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

    cleanup();
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

// Останов DiggerProxy и освобождение занятых в run() ресурсов
// --------------------------------------------------------------------------------
void Digger::cleanup()
{
  try
  {
    // Остановить DiggerProxy, послав служебное сообщение его управляющему сокету.
    // При этом DiggerProxy:
    // 1. Пошлет сообщение о завершении работы своим DiggerWorker-ам.
    // 2. Завершит все нити DiggerWorker
    // 3. Освободит все ресурсы, закрыв сокеты
    proxy_terminate();

    // Дождались останова
    LOG(INFO) << "Waiting joinable DiggerProxy thread";
    while (true)
    {
      if (m_proxy_thread->joinable())
      {
        m_proxy_thread->join();
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
    // NB: ресурсов в конструкторе не создавалось.
    // Они были созданы и освобождены в другой нити, в функции DiggerProxy::run()
    delete m_digger_proxy;
  }
  catch(std::exception &e)
  {
    LOG(ERROR) << e.what();
  }
}

// Пауза прокси-треда
// --------------------------------------------------------------------------------
void Digger::proxy_pause()
{
  try
  {
    LOG(INFO) << "Send PAUSE to DiggerProxy";
    m_helpers_control.send("PAUSE", 5, 0);
  }
  catch(std::exception &e)
  {
    LOG(ERROR) << e.what();
  }
}

// Продолжить исполнение прокси-треда
// --------------------------------------------------------------------------------
void Digger::proxy_resume()
{
  try
  {
    LOG(INFO) << "Send RESUME to DiggerProxy";
    m_helpers_control.send("RESUME", 6, 0);
  }
  catch(std::exception &e)
  {
    LOG(ERROR) << e.what();
  }
}

// Проверить работу прокси-треда
// --------------------------------------------------------------------------------
void Digger::proxy_probe()
{
  try
  {
    LOG(INFO) << "Send PROBE to DiggerProxy";
    m_helpers_control.send("PROBE", 5, 0);
  }
  catch(std::exception &e)
  {
    LOG(ERROR) << e.what();
  }
}


// Завершить работу прокси-треда
// --------------------------------------------------------------------------------
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


// --------------------------------------------------------------------------------
// NB : Функция обрабатывает полученное сообщение.
// Подразумевается обработка только служебных сообщений, не требующих подключения к БДРВ.
// Сейчас (2015.07.06) эта функция принимает запросы на доступ к БД, но не обрабатывает их.
int Digger::handle_request(mdp::zmsg* request, std::string*& reply_to)
{
  rtdbMsgType msgType;

  assert (request->parts () >= 2);
  LOG(INFO) << "Process new request with " << request->parts() 
            << " parts and reply to " << *reply_to;

  msg::Letter *letter = m_message_factory->create(request);
  if (letter->valid())
  {
    msgType = letter->header()->usr_msg_type();

    switch(msgType)
    {
//
// Здесь следует обрабатывать только запросы общего характера, не требующие подключения к БД
// К ним относятся все запросы, влияющие на общее функционирование и управление.
// В это время, запросы к БД обрабатываются в фоне нитями DiggerWorker-ов.
//
//      case SIG_D_MSG_READ_MULTI:
//       handle_read(letter, reply_to);
//       break;

//      case SIG_D_MSG_WRITE_MULTI:
//       handle_write(letter, reply_to);
//       break;

      case ADG_D_MSG_ASKLIFE:
       handle_asklife(letter, reply_to);
       break;

      default:
       LOG(ERROR) << "Unsupported request type: " << msgType;
    }
  }
  else
  {
    LOG(ERROR) << "Readed letter "<<letter->header()->exchange_id()<<" not valid";
  }

  delete letter;
  return 0;
}


// отправить адресату сообщение о статусе исполнения команды
// --------------------------------------------------------------------------------
void Digger::send_exec_result(int exec_val, std::string* reply_to)
{
  std::string OK = "хорошо";
  std::string FAIL = "плохо";
  msg::ExecResult *msg_exec_result = 
        dynamic_cast<msg::ExecResult*>(m_message_factory->create(ADG_D_MSG_EXECRESULT));
  mdp::zmsg       *response = new mdp::zmsg();

  msg_exec_result->set_destination(*reply_to);
  msg_exec_result->set_exec_result(exec_val);
  msg_exec_result->set_failure_cause(0, OK);

  response->push_front(const_cast<std::string&>(msg_exec_result->data()->get_serialized()));
  response->push_front(const_cast<std::string&>(msg_exec_result->header()->get_serialized()));
  response->wrap(reply_to->c_str(), "");

  std::cout << "Send ExecResult to " << *reply_to
            << " with status:" << msg_exec_result->exec_result()
            << " sid:" << msg_exec_result->header()->exchange_id()
            << " iid:" << msg_exec_result->header()->interest_id()
            << " dest:" << msg_exec_result->header()->proc_dest()
            << " origin:" << msg_exec_result->header()->proc_origin() << std::endl;

  LOG(INFO) << "Send ExecResult to " << *reply_to
            << " with status:" << msg_exec_result->exec_result()
            << " sid:" << msg_exec_result->header()->exchange_id()
            << " iid:" << msg_exec_result->header()->interest_id()
            << " dest:" << msg_exec_result->header()->proc_dest()
            << " origin:" << msg_exec_result->header()->proc_origin();

#warning "replace send_to_broker(msg::zmsg*) to send(msg::Letter*)"
  // TODO: Для упрощения обработки сообщения не следует создавать zmsg и заполнять его
  // данными из Letter, а сразу напрямую отправлять Letter потребителю.
  // Возможно, следует указать тип передачи - прямой или через брокер.
  send_to_broker((char*) MDPW_REPORT, NULL, response);

  delete response;
}

// --------------------------------------------------------------------------------
int Digger::handle_asklife(msg::Letter* letter, std::string* reply_to)
{
  msg::AskLife   *msg_ask_life = static_cast<msg::AskLife*>(letter);
  mdp::zmsg      *response = new mdp::zmsg();
  int exec_val = 1;

  msg_ask_life->set_exec_result(exec_val);

  response->push_front(const_cast<std::string&>(msg_ask_life->data()->get_serialized()));
  response->push_front(const_cast<std::string&>(msg_ask_life->header()->get_serialized()));
  response->wrap(reply_to->c_str(), "");

#if 0
  std::cout << "Processing asklife from " << *reply_to
            << " has status:" << msg_ask_life->exec_result(exec_val)
            << " sid:" << msg_ask_life->header()->exchange_id()
            << " iid:" << msg_ask_life->header()->interest_id()
            << " dest:" << msg_ask_life->header()->proc_dest()
            << " origin:" << msg_ask_life->header()->proc_origin() << std::endl;
#endif

  LOG(INFO) << "Processing asklife from " << *reply_to
            << " has status:" << msg_ask_life->exec_result(exec_val)
            << " sid:" << msg_ask_life->header()->exchange_id()
            << " iid:" << msg_ask_life->header()->interest_id()
            << " dest:" << msg_ask_life->header()->proc_dest()
            << " origin:" << msg_ask_life->header()->proc_origin();

#if 0
  // TODO: Т.к. запрос мог поступить не только от Брокера, но и напрямую от Клиента,
  // то здесь выбрать нужного получателя ответа.
  switch(m_
  {
    case DIRECT:
      send_direct((char*) MDPW_REPORT, NULL, response);
      break;

    case PERSISTENT:
#endif
      send_to_broker((char*) MDPW_REPORT, NULL, response);
#if 0
      break;
      
    default:
      LOG(ERROR) << "Unsupported messaging reply type: " << ;
  }
#endif
  delete response;

  return 0;
}

// --------------------------------------------------------------------------------
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

