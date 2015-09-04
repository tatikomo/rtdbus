#include <assert.h>
#include "glog/logging.h"
#include "google/protobuf/stubs/common.h"

#if defined HAVE_CONFIG_H
#include "config.h"
#endif

#include <unistd.h>
#include <sys/syscall.h>
#include <sys/resource.h>

#include <vector>
#include <thread>
#include <memory>
#include <functional>

//1 #include "helper.hpp"
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
// класс измерения производительности
#include "tool_metrics.hpp"

using namespace mdp;

extern int interrupt_worker;

// Таймаут (мсек) ожидания получения данных в экземпляре DiggerWorker
const int DiggerWorker::PollingTimeout = 1000;
const int DiggerProbe::kMaxThread = 8;
bool      DiggerProbe::m_probe_continue = true;
const int Digger::DatabaseSizeBytes = 1024 * 1024 * DIGGER_DB_SIZE_MB;

// --------------------------------------------------------------------------------
DiggerWorker::DiggerWorker(zmq::context_t &ctx, int sock_type, xdb::RtEnvironment* env) :
 m_context(ctx),
 m_worker(m_context, sock_type), // клиентский сокет для приема от фронтенда
 m_commands(m_context, ZMQ_SUB), // серверный сокет приема команд от DiggerProxy
 m_interrupt(false),
 m_environment(env),
 m_db_connection(NULL),
 m_message_factory(NULL),
 m_thread_id(0)
{
  LOG(INFO) << "DiggerWorker created";
  m_metric_center = new tool::Metrics();
}

// --------------------------------------------------------------------------------
DiggerWorker::~DiggerWorker()
{
  LOG(INFO) << "DiggerWorker destroyed";
  delete m_metric_center;
  delete m_db_connection;
  delete m_message_factory;
}

int DiggerWorker::probe(mdp::zmsg* request)
{
  char str_buffer[200];

/*
  sprintf(str_buffer, "tid:%d rss:%ld b2r:%g pd:%g u%:%g u#:%g s%:%g s#:%g",
          m_thread_id,
          m_metric_center->min().ru_rss,
          m_metric_center->min().between_2_requests,
          m_metric_center->min().processing_duration,
          m_metric_center->min().ucpu_usage_pct,
          m_metric_center->min().ucpu_usage,
          m_metric_center->min().scpu_usage_pct,
          m_metric_center->min().scpu_usage);

  LOG(INFO) << "STAT MIN " << str_buffer;
*/
  sprintf(str_buffer, "tid:%d rss:%ld b2r:%g pd:%g u%:%g u#:%g s%:%g s#:%g",
          m_thread_id,
          m_metric_center->min().ru_rss,
          m_metric_center->min().between_2_requests,
          m_metric_center->min().processing_duration,
          m_metric_center->min().ucpu_usage_pct,
          m_metric_center->min().ucpu_usage,
          m_metric_center->min().scpu_usage_pct,
          m_metric_center->min().scpu_usage);

  LOG(INFO) << "STAT AVG " << str_buffer;
/*
  sprintf(str_buffer, "tid:%d rss:%ld b2r:%g pd:%g u%:%g u#:%g s%:%g s#:%g",
          m_thread_id,
          m_metric_center->min().ru_rss,
          m_metric_center->min().between_2_requests,
          m_metric_center->min().processing_duration,
          m_metric_center->min().ucpu_usage_pct,
          m_metric_center->min().ucpu_usage,
          m_metric_center->min().scpu_usage_pct,
          m_metric_center->min().scpu_usage);

  LOG(INFO) << "STAT MAX " << str_buffer; */
//1  request->push_back();
}

// Нитевой обработчик запросов
// Выход из функции:
// 1. по исключению
// 2. получение строки идентификации, равной "TERMINATE"
// --------------------------------------------------------------------------------
void DiggerWorker::work()
{
   int linger = 0;
   int send_timeout_msec = 1000000;
   int recv_timeout_msec = 3000000;
   zmsg *request, *command = NULL;
   zmq::pollitem_t  socket_items[2];
   time_t last_probe_time, cur_time;

   m_thread_id = static_cast<long int>(syscall(SYS_gettid));

   m_worker.connect(ENDPOINT_SINF_DATA_BACKEND);
   m_worker.setsockopt(ZMQ_LINGER, &linger, sizeof (linger));
   m_worker.setsockopt(ZMQ_SNDTIMEO, &send_timeout_msec, sizeof(send_timeout_msec));
   m_worker.setsockopt(ZMQ_RCVTIMEO, &recv_timeout_msec, sizeof(recv_timeout_msec));

   m_commands.connect(ENDPOINT_SINF_COMMAND_BACKEND);
   m_commands.setsockopt(ZMQ_SUBSCRIBE, "", 0);
   m_commands.setsockopt(ZMQ_LINGER, &linger, sizeof (linger));
   m_commands.setsockopt(ZMQ_SNDTIMEO, &send_timeout_msec, sizeof(send_timeout_msec));
   m_commands.setsockopt(ZMQ_RCVTIMEO, &recv_timeout_msec, sizeof(recv_timeout_msec));

   LOG(INFO) << "DiggerWorker thread connects to "
             << ENDPOINT_SINF_DATA_BACKEND
             << ", " << ENDPOINT_SINF_COMMAND_BACKEND;


   // Сокет для получения запросов на данные
   // Источник поступления запросов - zmq_proxy из DiggerProxy,
   // в свою очередь получающий запросы от Клиентов.
   socket_items[0].socket = (void*)m_worker;
   socket_items[0].fd = 0;
   socket_items[0].events = ZMQ_POLLIN;
   socket_items[0].revents = 0;

   socket_items[1].socket = (void*)m_commands;
   socket_items[1].fd = 0;
   socket_items[1].events = ZMQ_POLLIN;
   socket_items[1].revents = 0;

   // Запомним как первоначальное значение
   last_probe_time = cur_time = time(0);

   try {
     m_message_factory = new msg::MessageFactory(DIGGER_NAME);

     // Каждая нить процесса, желающая работать с БДРВ,
     // должна получить свой экземпляр RtConnection
     m_db_connection = m_environment->getConnection();

     while (!m_interrupt)
     {
       zmq::poll (socket_items, 2, PollingTimeout);

       if (socket_items[0].revents & ZMQ_POLLIN) { // Получен запрос

          request = new zmsg (m_worker);
          assert (request);

          // replay address
          std::string identity  = request->pop_front();

          std::string empty = request->pop_front();

          processing(request, identity);

          delete request;
        } // если получен запрос

        if (socket_items[1].revents & ZMQ_POLLIN) { // Получена команда

          command = new zmsg (m_commands);
          assert (command);

          // Анализ названия команды
          std::string command_name  = command->pop_front();

          // Завершить работу
          if ((command_name.size() == 9) && (command_name.compare("TERMINATE") == 0))
          {
              LOG(INFO) << "Got TERMINATE command, shuttingdown this DiggerWorker thread";
              m_interrupt = true;
              delete command;
              continue;
          }
          // Отправить статистику использования
          if ((command_name.size() == 5) && (command_name.compare("PROBE") == 0))
          {
              LOG(INFO) << "Got PROBE command, send statistics info about this DiggerWorker thread";
              probe(command);
              last_probe_time = time(0);
              m_interrupt = false;
              delete command;
              continue;
          }
        } // если получена команда

        // Если главная нить аварийно завершилась -> самостоятельно прекратить работу
        cur_time = time(0);
        // Опросов не было в течении удвоенного штатного времени опроса.
        if (cur_time > (last_probe_time + POLLING_PROBE_PERIOD*2))
        {
          LOG(ERROR) << "Detect losing probe polling from DiggerProbe (last probe was "
                     << (cur_time - last_probe_time)
                     << " sec ago), exiting";
          m_interrupt = true;
          break;
        }
     }
   }
   catch (std::exception &e) {
     LOG(ERROR) << "DiggerWorker catch: " << e.what();
     m_interrupt = true;
   }
   catch(zmq::error_t error) {
     LOG(ERROR) << "DiggerWorker catch: " << error.what();
     m_interrupt = true;
   }

   m_worker.close();
   m_commands.close();

   delete m_db_connection; m_db_connection = NULL;
   delete m_message_factory; m_message_factory = NULL;

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

  // Получить отметку времени начала обработки запроса
  m_metric_center->before();

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
    LOG(ERROR) << "Received letter "<<letter->header()->exchange_id()<<" not valid";
  }

  delete letter;

  // Получить отметку времени завершения обработки запроса
  m_metric_center->after();

  return 0;
}

// --------------------------------------------------------------------------------
int DiggerWorker::handle_read(msg::Letter* letter, std::string& identity)
{
  msg::ReadMulti *read_msg = dynamic_cast<msg::ReadMulti*>(letter);
  xdb::RtPoint   *point = NULL;
  mdp::zmsg      *response = new mdp::zmsg();
  xdb::DbType_t  given_xdb_type;
  // удалить
  std::string delme_name;
  xdb::DbType_t delme_type;
  xdb::AttrVal_t delme_val;
  xdb::Quality_t delme_quality = xdb::ATTR_OK;

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



    if (true == read_msg->get(idx, delme_name, delme_type, delme_quality, delme_val))
    {
#if defined VERBOSE
        if (xdb::ATTR_OK == delme_quality)
        {
          std::cout << "[  OK  ] #" << idx << " name:" << delme_name << " type:" << delme_type << " val:";
        }
#endif

        switch(delme_type)
        {
#if defined VERBOSE
            case xdb::DB_TYPE_LOGICAL:
            std::cout << (signed int)delme_val.fixed.val_bool;
            break;
            case xdb::DB_TYPE_INT8:
            std::cout << (signed int)delme_val.fixed.val_int8;
            break;
            case xdb::DB_TYPE_UINT8:
            std::cout << (unsigned int)delme_val.fixed.val_uint8;
            break;
            case xdb::DB_TYPE_INT16:
            std::cout << delme_val.fixed.val_int16;
            break;
            case xdb::DB_TYPE_UINT16:
            std::cout << delme_val.fixed.val_uint16;
            break;
            case xdb::DB_TYPE_INT32:
            std::cout << delme_val.fixed.val_int32;
            break;
            case xdb::DB_TYPE_UINT32:
            std::cout << delme_val.fixed.val_uint32;
            break;
            case xdb::DB_TYPE_INT64:
            std::cout << delme_val.fixed.val_int64;
            break;
            case xdb::DB_TYPE_UINT64:
            std::cout << delme_val.fixed.val_uint64;
            break;
            case xdb::DB_TYPE_FLOAT:
            std::cout << delme_val.fixed.val_float;
            break;
            case xdb::DB_TYPE_DOUBLE:
            std::cout << delme_val.fixed.val_double;
            break;
#endif
            case xdb::DB_TYPE_BYTES:
#if defined VERBOSE
              std::cout << delme_val.dynamic.val_string;
#endif
              delete delme_val.dynamic.val_string;
            break;
            case xdb::DB_TYPE_BYTES4:
            case xdb::DB_TYPE_BYTES8:
            case xdb::DB_TYPE_BYTES12:
            case xdb::DB_TYPE_BYTES16:
            case xdb::DB_TYPE_BYTES20:
            case xdb::DB_TYPE_BYTES32:
            case xdb::DB_TYPE_BYTES48:
            case xdb::DB_TYPE_BYTES64:
            case xdb::DB_TYPE_BYTES80:
            case xdb::DB_TYPE_BYTES128:
            case xdb::DB_TYPE_BYTES256:
#if defined VERBOSE
              std::cout << delme_val.dynamic.varchar;
#endif
              delete [] delme_val.dynamic.varchar;
            break;

#if defined VERBOSE
            case xdb::DB_TYPE_ABSTIME:
              given_time = delme_val.fixed.val_time.tv_sec;
              strftime(s_date, D_DATE_FORMAT_LEN, D_DATE_FORMAT_STR, localtime(&given_time));
              snprintf(buffer, D_DATE_FORMAT_W_MSEC_LEN, "%s.%06ld", s_date, delme_val.fixed.val_time.tv_usec);
              std::cout << buffer;
            break;

            case xdb::DB_TYPE_UNDEF:

              switch(delme_quality)
              {
                case xdb::ATTR_OK:
                // Точка с таким тегом найдена, но ее тип действительно неопределен
                break;
                case xdb::ATTR_NOT_FOUND:
                // Точка с таким тегом не была найдена
                break;
                case xdb::ATTR_ERROR:
                // Возникла ошибка при чтении значения атрибута Точки
                break;
                default:
                break;
              }
            break;

            default:
              std::cout << "<unknown type>" << delme_type;
#endif
        }
#if defined VERBOSE
        std::cout << std::endl;
#endif
    }
    else // Ошибка функции get()
    {
#if defined VERBOSE
      LOG(ERROR) << "[ FAIL ] #" << idx << " name:" << delme_name << " type:" << delme_type << std::endl;
#endif
    }
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
DiggerProbe::DiggerProbe(zmq::context_t &ctx, xdb::RtEnvironment* env) :
 m_context(ctx),
 m_worker_command_socket(m_context, ZMQ_PUB),
 m_environment(env)
{
  LOG(INFO) << "DiggerProbe start, new context " << &m_context;
}

DiggerProbe::~DiggerProbe()
{
  LOG(INFO) << "DiggerProbe stop";
}

// Создать экземпляры Обработчиков и запустить их нити
// Вернуть количество запущенных нитей
int DiggerProbe::start_workers()
{
  int linger = 0;
  int send_timeout_msec = 1000000; // 1 sec
  int recv_timeout_msec = 3000000; // 3 sec

  try
  {
    // Для inproc создать точку подключения к нитям Обработчиков ДО вызова connect в Клиентах
    m_worker_command_socket.bind(ENDPOINT_SINF_COMMAND_BACKEND);
    m_worker_command_socket.setsockopt(ZMQ_LINGER, &linger, sizeof (linger));
    m_worker_command_socket.setsockopt(ZMQ_SNDTIMEO, &send_timeout_msec, sizeof(send_timeout_msec));
    m_worker_command_socket.setsockopt(ZMQ_RCVTIMEO, &recv_timeout_msec, sizeof(recv_timeout_msec));

    for (int i = 0; i < kMaxThread; ++i)
    {
      DiggerWorker *p_dw = new DiggerWorker(m_context, ZMQ_DEALER, m_environment);
      std::thread *p_dwt = new std::thread(std::bind(&DiggerWorker::work, p_dw));
      m_worker_list.push_back(p_dw);
      m_worker_thread.push_back(p_dwt);
      LOG(INFO) << "created DiggerWorker["<<i<<"] " << p_dw << ", thread " << p_dwt;
      //LOG(INFO) << "created DiggerWorker["<<i<<"] " << m_worker_list[i] << ", thread " << worker_thread[i];
    }
  }
  catch(zmq::error_t error)
  {
    LOG(ERROR) << "DiggerProbe catch: " << error.what();
  }
  catch (std::exception &e)
  {
    LOG(ERROR) << "DiggerProbe catch the signal: " << e.what();
  }

  return m_worker_list.size();
}

// Остановить нити Обработчиков.
// Вызывается из DiggerProbe::work при значении false переменной m_probe_continue.
// Значение m_probe_continue меняется из DiggerProxy::work() при возврате из zmq_proxy
// при получении ей сигнала TERMINATE.
void DiggerProbe::shutdown_workers()
{
  // Количество оставшихся в работе нитей DiggerWorker, уменьшается от kMaxThread до 0
  int threads_to_join;

  try
  {
      // Вызвать останов функции DiggerWorker::work() для завершения треда
      for (int i = 0; i < kMaxThread; ++i)
      {
        LOG(INFO) << "Send TERMINATE to DiggerWorker " << i;
        m_worker_command_socket.send("TERMINATE", 9, 0);
      }

      LOG(INFO) << "DiggerProbe will cleanup "<<m_worker_list.size()
                <<" threads after 100 msec with timeout 5 sec";
      usleep(100000);

      threads_to_join = m_worker_thread.size();
      time_t when_joining_starts = time(0);
      while (threads_to_join)
      {
        LOG(INFO) << "Wait to join " << threads_to_join << " DiggerWorkers";
        for (std::vector<std::thread*>::iterator it = m_worker_thread.begin();
             it != m_worker_thread.end();
             ++it)
        {
          if ((*it)->joinable())
          {
            LOG(INFO) << "join DiggerWorker[" << threads_to_join-- << "] dwt=" << (*it);
            (*it)->join();
            delete (*it);
          }
          else
          {
            // TODO: оформить время таймаута в нормальном виде (конфиг?)
            if (time(0) > (when_joining_starts + 5))
            {
              LOG(ERROR) << "Timeout exceeds to joint DiggerProxy threads!";
            }
            LOG(INFO) << "Skip joining thread " << *it;
            usleep(100000);
          }
        }
      }
      
      for(unsigned int i = 0; i < m_worker_list.size(); ++i)
      {
        LOG(INFO) << "delete DiggerWorker["<<i+1<<"/"<<m_worker_list.size()<<"] " << m_worker_list[i];
        delete m_worker_list[i];
      }
  }
  catch(zmq::error_t error)
  {
      LOG(ERROR) << "DiggerProbe catch: " << error.what();
  }
  catch (std::exception &e)
  {
      LOG(ERROR) << "DiggerProbe catch the signal: " << e.what();
  }
}

// --------------------------------------------------------------------------------
// Отдельная нить бесконечной проверки состояния и метрик нитей DiggerWorker
//  Условия завершения работы:
//  1. Исключение zmq::error
//  2. Установка флага m_probe_continue в false
//
//  Необходимо опрашивать экземпляры DiggerWorker для:
//  1. Определения их работоспособности
//  2. Получения метрик обслуживания запросов
//  (min/avg/max, среднее время между запросами, ...)
//  --------------------------------------------------------------------------------
void DiggerProbe::work()
{
  xdb::RtConnection *db_connection = NULL;

  try
  {
    db_connection = m_environment->getConnection();

    while (m_probe_continue)
    {
//      LOG(INFO) << "Probe!";
      // Отправить сообщение PROBE всем экземплярам DiggerWorker
      m_worker_command_socket.send("PROBE", 5, 0);

#if 0
      // И дождаться ответа от каждого
      for (std::vector<DiggerWorker*>::iterator it = m_worker_list.begin();
           it != m_worker_list.end();
           ++it)
      {
//        (*it)->probe(NULL);
      }
#endif
      
      sleep(POLLING_PROBE_PERIOD);
    }

    // Послать сигнал TERMINATE Обработчикам и дождаться их завершения
    shutdown_workers();
  }
  catch(zmq::error_t error)
  {
    LOG(ERROR) << "DiggerProbe catch: " << error.what();
  }
  catch (std::exception &e)
  {
    LOG(ERROR) << "DiggerProbe catch the signal: " << e.what();
  }

  m_worker_command_socket.close();

  delete db_connection;

  pthread_exit(NULL);
}

void DiggerProbe::stop()
{
  LOG(INFO) << "Probe got signal to stop";
  m_probe_continue = false;
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
  int num_running_threads;
  int mandatory = 1;
  int linger = 0;
  int hwm = 100;
  int send_timeout_msec = 1000000; // 1 sec
  int recv_timeout_msec = 3000000; // 3 sec

  try
  {
    // Сокет прямого подключения к экземплярям DiggerWorker
    // NB: Выполняется в отдельной нити 
    //ZMQ_ROUTER_MANDATORY может привести zmq_proxy_steerable к аномальному завершению: rc=-1, errno=113
    //Наблюдалось в случаях интенсивного обмена брокера с клиентом, если последний аномально завершался.
    m_frontend.setsockopt(ZMQ_ROUTER_MANDATORY, &mandatory, sizeof (mandatory));
#warning "Проверь связь между ZMQ_ROUTER_MANDATORY и завершением zmq_proxy_steerable с errno=113"
    m_frontend.bind("tcp://lo:5556" /*ENDPOINT_SINF_FRONTEND*/);
    m_frontend.setsockopt(ZMQ_LINGER, &linger, sizeof (linger));
    m_frontend.setsockopt(ZMQ_RCVHWM, &hwm, sizeof(hwm));
    m_frontend.setsockopt(ZMQ_SNDHWM, &hwm, sizeof(hwm));
    m_frontend.setsockopt(ZMQ_SNDTIMEO, &send_timeout_msec, sizeof(send_timeout_msec));
    m_frontend.setsockopt(ZMQ_RCVTIMEO, &recv_timeout_msec, sizeof(recv_timeout_msec));
    LOG(INFO) << "DiggerProxy binds to DiggerWorkers frontend " << ENDPOINT_SINF_FRONTEND;
    m_backend.bind(ENDPOINT_SINF_DATA_BACKEND);
    m_backend.setsockopt(ZMQ_LINGER, &linger, sizeof (linger));
    m_backend.setsockopt(ZMQ_SNDTIMEO, &send_timeout_msec, sizeof(send_timeout_msec));
    m_backend.setsockopt(ZMQ_RCVTIMEO, &recv_timeout_msec, sizeof(recv_timeout_msec));
    LOG(INFO) << "DiggerProxy binds to DiggerWorkers backend " << ENDPOINT_SINF_DATA_BACKEND;

    // Настройка управляющего сокета
    LOG(INFO) << "DiggerProxy is ready to connect with DiggerWorkers control";
    m_control.connect(ENDPOINT_SINF_PROXY_CTRL);
    LOG(INFO) << "DiggerProxy connects to DiggerWorkers control " << ENDPOINT_SINF_PROXY_CTRL;
    m_control.setsockopt(ZMQ_SUBSCRIBE, "", 0);
    m_control.setsockopt(ZMQ_LINGER, &linger, sizeof (linger));
    m_control.setsockopt(ZMQ_SNDTIMEO, &send_timeout_msec, sizeof(send_timeout_msec));
    m_control.setsockopt(ZMQ_RCVTIMEO, &recv_timeout_msec, sizeof(recv_timeout_msec));

    // Создать объект DiggerProbe для управления нитями DiggerWorker
    LOG(INFO) << "DiggerProxy starting DiggerProbe thread";
    m_probe = new DiggerProbe(m_context, m_environment);

    // Создать пул Обработчиков Службы в DiggerProbe
    if (DiggerProbe::kMaxThread == (num_running_threads = m_probe->start_workers()))
    {
      // Количество запущенных процессов совпадает с заданным.
      // Запустить нить управления DiggerProbe.
      m_probe_thread = new std::thread(std::bind(&DiggerProbe::work, m_probe));

      LOG(INFO) << "DiggerProxy (" << ENDPOINT_SINF_FRONTEND
                << ", " << ENDPOINT_SINF_DATA_BACKEND << ")";

      int rc = zmq_proxy_steerable (m_frontend, m_backend, nullptr, m_control);
      if (0 == rc)
      {
        LOG(INFO) << "DiggerProxy zmq::proxy finish successfuly";
      }
      else
      {
        LOG(ERROR) << "DiggerProxy zmq::proxy failure, errno=" << errno
                   << ", rc=" << rc;
        //throw error_t ();
      }

      // Остановить нити Обработчиков, затем нить Probe и дождаться ее останова
      wait_probe_stop();
    }
    else
    {
      LOG(ERROR) << "Start less DiggerWorker's threads than needed: "
                 << m_probe_thread << " of " << DiggerProbe::kMaxThread;
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

  m_frontend.close();
  m_backend.close();
  m_control.close();

  interrupt_worker = true;

  pthread_exit(NULL);
}


// Контекст вызова - DiggerProxy::run()
void DiggerProxy::wait_probe_stop()
{
  // Ипфнормировать Пробник о необходимости завершения его работы
  m_probe->stop();

  try
  {
    time_t when_joining_starts = time(0);
    // Дождаться останова
    LOG(INFO) << "Waiting joinable Digger Probe thread";
    while (true)
    {
      if (m_probe_thread->joinable())
      {
        m_probe_thread->join();
        LOG(INFO) << "thread Digger Probe joined";
        break;
      }
      else
      {
        // TODO: оформить время таймаута в нормальном виде (конфиг?)
        if (time(0) > (when_joining_starts + 5))
        {
          LOG(ERROR) << "Timeout exceeds to joint DiggerProxy Probe thread!";
        }
        usleep(500000);
        LOG(INFO) << "tick join Digger Probe waiting thread";
      }
    }
  }
  catch(zmq::error_t error)
  {
    LOG(ERROR) << "DiggerProxy Probe catch: " << error.what();
  }
  catch (std::exception &e)
  {
    LOG(ERROR) << "DiggerProxy Probe catch the signal: " << e.what();
  }

  delete m_probe;
  delete m_probe_thread;
}

// Класс-расширение штатной Службы для обработки запросов к БДРВ
// --------------------------------------------------------------------------------
Digger::Digger(std::string broker_endpoint, std::string service, int verbose)
   :
   mdp::mdwrk(broker_endpoint, service, verbose, 2/* num zmq io threads (default = 1) */),
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
  m_appli->setOption("OF_MAX_CONNECTIONS",  DiggerProbe::kMaxThread + 4);
  m_appli->setOption("OF_RDWR",1);      // Открыть БД для чтения/записи
  m_appli->setOption("OF_DATABASE_SIZE",    Digger::DatabaseSizeBytes);
  m_appli->setOption("OF_MEMORYPAGE_SIZE",  1024);
  m_appli->setOption("OF_MAP_ADDRESS",      0x20000000);
#if defined USE_EXTREMEDB_HTTP_SERVER
  m_appli->setOption("OF_HTTP_PORT",        8083);
#endif
  m_appli->setOption("OF_DISK_CACHE_SIZE",  0);

  m_appli->initialize();
  //std::cout << "ZMQ_IO_THREADS=" << zmq_ctx_get((void*)m_context, ZMQ_IO_THREADS) << std::endl;
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
    m_environment = m_appli->loadEnvironment("SINF");
    // Каждая нить процесса, желающая работать с БДРВ, должна получить свой экземпляр
    m_db_connection = m_environment->getConnection();

    LOG(INFO) << "RTDB status: " << m_environment->getLastError().what();

    LOG(INFO) << "DiggerProxy creating, interrupt_worker=" << &interrupt_worker;
    m_digger_proxy = new DiggerProxy(m_context, m_environment);

    LOG(INFO) << "DiggerProxy starting thread";
    m_proxy_thread = new std::thread(std::bind(&DiggerProxy::run, m_digger_proxy));

    LOG(INFO) << "Wait 1 second to DiggerProxy became start";
    sleep(1);

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

  delete m_db_connection;   m_db_connection = NULL;
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

    LOG(INFO) << "Start service " << service_name;

    // Запуск нити DiggerProxy и нескольких DiggerWorker
    // ВЫполнение основного цикла работы приема-обработки-передачи сообщений
    engine->run();
  }
  catch(zmq::error_t err)
  {
    LOG(ERROR) << err.what();
  }
  delete engine;
  LOG(INFO) << "Finish service " << service_name;

  ::google::protobuf::ShutdownProtobufLibrary();
  ::google::ShutdownGoogleLogging();
  return 0;
}
#endif

