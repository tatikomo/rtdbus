// Общесистемные заголовочные файлы
#include <assert.h>
#include <unistd.h>
#include <sys/syscall.h>
#include <sys/resource.h>

#include <vector>
#include <unordered_set>
#include <thread>
#include <memory>
#include <functional>

// Служебные заголовочные файлы сторонних утилит
#include "glog/logging.h"
#include "google/protobuf/stubs/common.h"

// Служебные файлы RTDBUS
#if defined HAVE_CONFIG_H
#include "config.h"
#endif
#include "xdb_rtap_application.hpp"
#include "xdb_rtap_environment.hpp"
#include "xdb_rtap_connection.hpp"
#include "xdb_rtap_point.hpp"

#include "wdigger.hpp"

#include "msg_common.h"
#include "mdp_worker_api.hpp"
#include "mdp_zmsg.hpp"

// общий интерфейс сообщений
#include "msg_common.h"
#include "msg_message.hpp"
// сообщения общего порядка
#include "msg_adm.hpp"
// сообщения по работе с БДРВ
#include "msg_sinf.hpp"
// класс измерения производительности
#include "tool_metrics.hpp"

using namespace mdp;

extern volatile int interrupt_worker;

// Таймаут в миллисекундах (мсек) ожидания получения данных в экземпляре DiggerWorker
const int DiggerWorker::PollingTimeout = 1000;
// Период в миллисекундах между опросами изменений в Группах подписки
const int DiggerPoller::PollingPeriod = 500;
const int DiggerProbe::kMaxWorkerThreads = 3;
const int Digger::DatabaseSizeBytes = 1024 * 1024 * DIGGER_DB_SIZE_MB;
// Статические флаги завершения работы
// NB: Каждый экземпляр DiggerWorker имеет локальный флаг останова
volatile bool DiggerProbe::m_interrupt = false;
volatile bool DiggerProxy::m_interrupt = false;
volatile bool DiggerPoller::m_interrupt = false;

// --------------------------------------------------------------------------------
bool publish_sbs_impl(std::string& dest_name,
                      xdb::SubscriptionPoints_t &info_all,
                      rtdbMsgType msg_type,
                      msg::MessageFactory *message_factory,
                      zmq::socket_t& pub_socket);
void release_point_info(xdb::PointDescription_t*);

// Общая часть реализации отправки SBS сообщения для классов DiggerPoller и DiggerWorker
// --------------------------------------------------------------------------------
bool publish_sbs_impl(std::string& dest_name,
                      xdb::SubscriptionPoints_t &info_all,
                      rtdbMsgType msg_type,
                      msg::MessageFactory *message_factory,
                      zmq::socket_t& pub_socket)
{
  bool status = false;
  mdp::zmsg sbs_message;
  xdb::AttributeMap_t::iterator it_attr;
  msg::SubscriptionEvent *bundle_se = NULL;
  msg::ReadMulti *bundle_rm = NULL;
  msg::Letter *letter = message_factory->create(msg_type);
  xdb::SubscriptionPoints_t::iterator it_point;
  std::string tag_with_attr;

  switch(msg_type)
  {
    case SIG_D_MSG_GRPSBS:
      // Пустое первоначально сообщение об изменениях в группе подписки
      bundle_se = dynamic_cast<msg::SubscriptionEvent*>(letter);
      break;

    case SIG_D_MSG_READ_MULTI:
      // Пустое первоначально сообщение о всех атрибутах группы подписки
      bundle_rm = dynamic_cast<msg::ReadMulti*>(letter);
      break;

    default:
      LOG(ERROR) << "Unsupported message type: " << msg_type;
      assert(0 == 1);
  }

  letter->set_destination(dest_name);

  try
  {
    LOG(INFO) << "SBS publish_sbs: "<<info_all.size()<<" tags for '" << dest_name << "' msg_type: "<<msg_type;
    for(it_point = info_all.begin(); it_point != info_all.end(); it_point++)
    {
      for(it_attr = (*it_point)->attributes.begin();
          it_attr != (*it_point)->attributes.end();
          it_attr++)
      {
        tag_with_attr.assign((*it_point)->tag);
        tag_with_attr += ".";
        tag_with_attr += (*it_attr).first;

        if (SIG_D_MSG_GRPSBS == msg_type)
          bundle_se->add(tag_with_attr, (*it_attr).second.type, static_cast<void*>(&(*it_attr).second.value));
        else if (SIG_D_MSG_READ_MULTI == msg_type)
          bundle_rm->add(tag_with_attr, (*it_attr).second.type, static_cast<void*>(&(*it_attr).second.value));
      }
    }

#if 0
    if (SIG_D_MSG_GRPSBS == msg_type)
    {
      // второй фрейм - изменившиеся данные в формате ValueUpdate
      sbs_message.push_front(const_cast<std::string&>(bundle_se->data()->get_serialized()));
      // первый фрейм - заголовок сообщения
      sbs_message.push_front(const_cast<std::string&>(bundle_se->header()->get_serialized()));
      // нулевой фрейм - название Группы Подписки
      LOG(INFO) << "GEV: SIG_D_MSG_GRPSBS, dest_name=" << dest_name;
    }
    else if (SIG_D_MSG_READ_MULTI == msg_type)
    {
      // второй фрейм - изменившиеся данные в формате ValueUpdate
      sbs_message.push_front(const_cast<std::string&>(bundle_rm->data()->get_serialized()));
      // первый фрейм - заголовок сообщения
      sbs_message.push_front(const_cast<std::string&>(bundle_rm->header()->get_serialized()));
      // ???
      LOG(INFO) << "GEV: SIG_D_MSG_READ_MULTI, dest_name=" << dest_name;
    }
#else
    // второй фрейм - изменившиеся данные в формате ValueUpdate
    sbs_message.push_front(const_cast<std::string&>(letter->data()->get_serialized()));
    // первый фрейм - заголовок сообщения
    sbs_message.push_front(const_cast<std::string&>(letter->header()->get_serialized()));
    LOG(INFO) << "GEV: msg #" << msg_type << ", dest_name=" << dest_name;
#endif

    sbs_message.wrap(dest_name.c_str(), EMPTY_FRAME);

    //1 sbs_message.dump();
    // публикация набора
    sbs_message.send(pub_socket);

    // исключений не было - считаем, что все хорошо
    status = true;
  }
  catch(zmq::error_t error)
  {
    LOG(ERROR) << "SBS processing catch: " << error.what();
  }
  catch (std::exception &e)
  {
    LOG(ERROR) << "SBS processing catch the signal: " << e.what();
  }

  delete letter;

  return status;
}

// --------------------------------------------------------------------------------
// Освободить занятые ресурсы
void release_point_info(xdb::PointDescription_t* info)
{
  assert(info);

  // Почистить память, выделенную для атрибутов
  for(xdb::AttributeMapIterator_t it = info->attributes.begin();
      it != info->attributes.end();
      it++)
  {
#warning "проверить вероятность двойного удаления value.dynamic"
      switch((*it).second.type)
      {
        case xdb::DB_TYPE_BYTES:     /* 12 */
          delete (*it).second.value.dynamic.val_string;
          break;

        case xdb::DB_TYPE_BYTES4:    /* 13 */
        case xdb::DB_TYPE_BYTES8:    /* 14 */
        case xdb::DB_TYPE_BYTES12:   /* 15 */
        case xdb::DB_TYPE_BYTES16:   /* 16 */
        case xdb::DB_TYPE_BYTES20:   /* 17 */
        case xdb::DB_TYPE_BYTES32:   /* 18 */
        case xdb::DB_TYPE_BYTES48:   /* 19 */
        case xdb::DB_TYPE_BYTES64:   /* 20 */
        case xdb::DB_TYPE_BYTES80:   /* 21 */
        case xdb::DB_TYPE_BYTES128:  /* 22 */
        case xdb::DB_TYPE_BYTES256:  /* 23 */
          delete [] (*it).second.value.dynamic.varchar;
          break;

        default: ; // другие типы - статическое выделение памяти
      }
  }

  delete info;
}


// --------------------------------------------------------------------------------
DiggerWorker::DiggerWorker(zmq::context_t *ctx, int sock_type, xdb::RtEnvironment* env) :
 m_context(ctx),
 m_worker(*m_context, sock_type), // клиентский сокет для приема от фронтенда
 m_commands(*m_context, ZMQ_SUB), // серверный сокет приема команд от DiggerProxy
 m_thread_id(0),
 m_interrupt(false),
 m_environment(env),
 m_db_connection(NULL),
 m_message_factory(NULL),
 m_metric_center(NULL)
{
  LOG(INFO) << "DiggerWorker created";
  m_metric_center = new tool::Metrics();
}

// --------------------------------------------------------------------------------
DiggerWorker::~DiggerWorker()
{
  LOG(INFO) << "start DiggerWorker destructor";
  delete m_metric_center;
  delete m_db_connection;
  delete m_message_factory;
  LOG(INFO) << "finish DiggerWorker destructor";
}

int DiggerWorker::probe(mdp::zmsg* request)
{
  int rc = NOK;
#if 1
#warning "DiggerWorker::Probe is temporarly suppressed"
  return OK;
#else
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
  sprintf(str_buffer, "tid(%d) rss(%ld) b2r(%g) pd(%g) u%%(%g) u#(%g) s%%(%g) s#(%g)",
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
  rc = OK;
#endif

  return rc;
}

// Нитевой обработчик запросов
// Выход из функции:
// 1. по исключению
// 2. получение строки идентификации, равной "TERMINATE"
// --------------------------------------------------------------------------------
void DiggerWorker::work()
{
   int linger = 0;
   int send_timeout_msec = SEND_TIMEOUT_MSEC;
   int recv_timeout_msec = RECV_TIMEOUT_MSEC;
   zmsg *request, *command = NULL;
   zmq::pollitem_t  socket_items[2];
   time_t last_probe_time, cur_time;

   m_thread_id = static_cast<long int>(syscall(SYS_gettid));

   m_worker.connect(ENDPOINT_RTDB_DATA_BACKEND);
   m_worker.setsockopt(ZMQ_LINGER, &linger, sizeof (linger));
   m_worker.setsockopt(ZMQ_SNDTIMEO, &send_timeout_msec, sizeof(send_timeout_msec));
   m_worker.setsockopt(ZMQ_RCVTIMEO, &recv_timeout_msec, sizeof(recv_timeout_msec));

   m_commands.connect(ENDPOINT_RTDB_COMMAND_BACKEND);
   m_commands.setsockopt(ZMQ_SUBSCRIBE, "", 0);
   m_commands.setsockopt(ZMQ_LINGER, &linger, sizeof (linger));
   m_commands.setsockopt(ZMQ_SNDTIMEO, &send_timeout_msec, sizeof(send_timeout_msec));
   m_commands.setsockopt(ZMQ_RCVTIMEO, &recv_timeout_msec, sizeof(recv_timeout_msec));

   LOG(INFO) << "DiggerWorker thread " << m_thread_id
             << " connects to " << ENDPOINT_RTDB_DATA_BACKEND
             << ", " << ENDPOINT_RTDB_COMMAND_BACKEND;


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
   catch(zmq::error_t error) {
     LOG(ERROR) << "DiggerWorker catch: " << error.what();
     m_interrupt = true;
   }
   catch (std::exception &e) {
     LOG(ERROR) << "DiggerWorker catch: " << e.what();
     m_interrupt = true;
   }

   m_worker.close();
   m_commands.close();

   delete m_db_connection; m_db_connection = NULL;
   delete m_message_factory; m_message_factory = NULL;

   LOG(INFO) << "DiggerWorker thread " << m_thread_id << " is done";
   pthread_exit(NULL);
}


// --------------------------------------------------------------------------------
// Функция первично обрабатывает полученный запрос на содержимое БДРВ.
// NB: Запросы к БД обрабатываются конкурентно, в фоне, нитями DiggerWorker-ов.
int DiggerWorker::processing(mdp::zmsg* request, std::string &identity)
{
  int rc = OK;
  rtdbMsgType msgType;

//  LOG(INFO) << "Process new request with " << request->parts() 
//            << " parts and reply to " << identity;

  // Получить отметку времени начала обработки запроса
  m_metric_center->before();

  msg::Letter *letter = m_message_factory->create(request);
  if (letter->valid())
  {
    msgType = letter->header()->usr_msg_type();

    switch(msgType)
    {
      // Множественное чтение
      case SIG_D_MSG_READ_MULTI:
        rc = handle_read(letter, identity);
        break;
      // Множественная запись
      case SIG_D_MSG_WRITE_MULTI:
        rc = handle_write(letter, identity);
        break;
      // Управление подписками
      case SIG_D_MSG_GRPSBS_CTRL:
        rc = handle_sbs_ctrl(letter, identity);
        break;
  #if 0 // Это сообщение должно обрабатываться уровнем выше, в Digger
      // Сообщение о состоянии
      case ADG_D_MSG_ASKLIFE:
        rc = handle_asklife(letter, identity);
        break;
  #endif

      default:
        LOG(ERROR) << "Unsupported request type: " << msgType;
        rc = NOK;
    }
  }
  else
  {
    LOG(ERROR) << "Received letter "<<letter->header()->exchange_id()<<" not valid";
    rc = NOK;
  }

  delete letter;

  // Получить отметку времени завершения обработки запроса
  m_metric_center->after();

  return rc;
}

// --------------------------------------------------------------------------------
int DiggerWorker::handle_read(msg::Letter* letter, std::string& identity)
{
  int rc = OK;
  msg::ReadMulti *read_msg = dynamic_cast<msg::ReadMulti*>(letter);
  mdp::zmsg      *response = new mdp::zmsg();
  xdb::DbType_t  given_xdb_type;
#if (VERBOSE > 2)
  struct tm result_time;
#endif
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

  for (std::size_t idx = 0; idx < read_msg->num_items(); idx++)
  {
    // todo содержит заново проинициализированный экземпляр типа msg::Value,
    // основываясь на данном пользователе типе данных. Если пользователь
    // ошибся, инициализация будет некорректной.
    //
    // Это критично для строковых типов атрибутов - память под данные
    // выделена не будет, при этом указатель на строковые данные будет
    // ненулевым из-за слитного размещения всех типов данных (union AttrVal_t).
    // Для них функция чтения правильно определит тип данных (строковое),
    // и посчитает, что память для строки уже была выделена.
    // В результате будет попытка записи в неправильную область памяти.
    msg::Value& todo = read_msg->item(idx);

    // Запомнить данный пользователем тип данных, он может отличаться от фактического
    given_xdb_type = todo.type();

    // Получить значение и фактический тип атрибута
    const xdb::Error& err = m_db_connection->read(&todo.instance());

    if (!err.Ok()) {
      // Нет такой точки в БДРВ
      LOG(ERROR) << "Request unexistent point \"" << todo.tag() << "\"";
      // rc = NOK;
    }
    else
    {
      // Обновить значение под контролем protobuf
      // Перенос прочитанных из БДРВ значений (AttrVal) в RTDBM::ValueUpdate
      todo.flush();
    
#if (VERBOSE > 2)
        LOG(INFO) << "Read [" << todo.type() << "] " << todo.tag() << " = " << todo.as_string();
#endif

      if ((given_xdb_type == xdb::DB_TYPE_UNDEF) && (todo.type() != xdb::DB_TYPE_UNDEF))
      {
        // Обновить значение типа атрибута, т.к. он неизвестен вызывающей стороне
        read_msg->set_type(idx, todo.type());
      }
    }

    if (true == read_msg->get(idx, delme_name, delme_type, delme_quality, delme_val))
    {
#if (VERBOSE > 2)
        if (xdb::ATTR_OK == delme_quality)
        {
          std::cout << "[  OK  ] #" << idx << " name:" << delme_name << " type:" << delme_type << " val:";
        }
#endif

        switch(delme_type)
        {
#if (VERBOSE > 3)
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
#if (VERBOSE > 2)
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
#if (VERBOSE > 2)
              std::cout << delme_val.dynamic.varchar;
#endif
              delete [] delme_val.dynamic.varchar;
            break;

#if (VERBOSE > 2)
            case xdb::DB_TYPE_ABSTIME:
              given_time = delme_val.fixed.val_time.tv_sec;
              localtime_r(&given_time, &result_time);
              strftime(s_date, D_DATE_FORMAT_LEN, D_DATE_FORMAT_STR, &result_time);
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
#else
            default: ; // nothing to do
#endif
        }
#if (VERBOSE > 2)
        std::cout << std::endl;
#endif
    }
    else // Ошибка функции get()
    {
#if (VERBOSE > 2)
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

  return rc;
}

// --------------------------------------------------------------------------------
int DiggerWorker::handle_write(msg::Letter* letter, std::string& identity)
{
  msg::WriteMulti *write_msg = dynamic_cast<msg::WriteMulti*>(letter);
  int status_code = 0;

  LOG(INFO) << "Processing WriteMulti ("<<write_msg->num_items()<<") request to " << identity;

  for (std::size_t idx = 0; idx < write_msg->num_items(); idx++)
  {
    msg::Value& todo = write_msg->item(idx);

#if (VERBOSE > 2)
    LOG(INFO) << "write #"<<idx<<":"<<todo.tag()<< ":"<<(unsigned int)todo.type()<<":"<<todo.as_string();
#endif
    // Изменить значение атрибута
    const xdb::Error& err = m_db_connection->write(&todo.instance());
    // Накапливающийся код ошибки
    status_code |= err.code();

    switch(err.code())
    {
      case xdb::rtE_NONE: // Все в порядке
        status_code = 0;
#if (VERBOSE > 2)
        std::cout << "[  OK  ] #" << idx << " name:" << todo.tag()
                  << " type:" << todo.type()
                  << " val:" << todo.as_string()
                  << std::endl;
#endif
      break;

      case xdb::rtE_ILLEGAL_PARAMETER_VALUE:
        LOG(ERROR) << "Writing \"" << todo.tag() << "\": " << err.what();
      break;

      case xdb::rtE_ATTR_NOT_FOUND: // Нет такой точки в БДРВ
        LOG(ERROR) << "Writing \"" << todo.tag() << "\": " << err.what();
      break;

      default:
        LOG(ERROR) << "Writing \"" << todo.tag()
                   << "\": Unsupported error code: " << err.what();
    }
  }

  send_exec_result(status_code, identity);
  return status_code;
}

// --------------------------------------------------------------------------------
int DiggerWorker::handle_sbs_ctrl(msg::Letter* letter, std::string& identity)
{
  xdb::rtDbCq operation;
  xdb::Error result;
  xdb::SubscriptionPoints_t points_list;
  xdb::SubscriptionPoints_t::iterator it_points;
  xdb::AttributeMap_t::iterator it_attr;
  int list_size = 1;
  std::string sbs_name;
  // Полученное сообщение
  msg::SubscriptionControl *sbs_ctrl_msg = dynamic_cast<msg::SubscriptionControl*>(letter);
  int status_code = 0;
  bool do_continue = true;

  LOG(INFO) << "Processing SubscriptionControl('"<<sbs_ctrl_msg->name()
            << "', " <<sbs_ctrl_msg->ctrl()
            << ") request to " << identity;

  // 1. Активировать подписку с указанным именем, если она в состоянии SUSPEND
  // TODO: определить, если ли еще подписчики данной группы? Скорее всего,
  // запуск/останов подписки должен/может осуществляться только сервером Групп
  // подписки, Клиенты этого делать не должны.
  //
  // 2. Прочитать текущие значения атрибутов из группы подписки
  //
  // 3. Отправить сообщение SIG_D_MSG_READ_MULTI, содержащее набор значений
  // всех атрибутов данной группы.
  //
  // NB: В качестве первого сообщения от сервера групп подписок используется SIG_D_MSG_READ_MULTI,
  // для различения двух стадий - однократной инициализации (1) от DiggerWorker (SIG_D_MSG_READ_MULTI
  // по тому же каналу, что и полученный запрос), и периодических обновлений (2) от DiggerPoller
  // (SIG_D_MSG_SBSGRP по каналу PUB-SUB).

  // 1 ======================================
  sbs_name.assign(sbs_ctrl_msg->name());

  switch(sbs_ctrl_msg->ctrl())
  {
    case 0:
      operation.action.config = xdb::rtCONFIG_SUSPEND_GROUP_SBS;
      break;

    case 1:
      operation.action.config = xdb::rtCONFIG_ENABLE_GROUP_SBS;
      break;

    default:
      LOG(ERROR) << "Unsupported CONTROL ("<<sbs_ctrl_msg->ctrl()<<") for "<< sbs_name;
      do_continue = false;
  }

  if (do_continue)
  {
      operation.buffer = static_cast<void*>(&sbs_name);
      // NB: 2015-10-08 команды SUSPEND и ENABLE не реализованы, возвращают ошибку 
      result = m_db_connection->ConfigDatabase(operation);

      // 2 ======================================
      // Передаем в качестве размера массива 1, поскольку хотя мы еще не знаем
      // точного количества точек в этой группе, массив точек points_list будет
      // расширяться динамически.
      result = m_db_connection->read(sbs_name, &list_size, &points_list);
      if (result.Ok())
      {
        // Отправить подписчикам все значения атрибутов
        if (false == publish_sbs_impl(identity, // название адресата
                points_list,                    // перечень всех атрибутов со значениями
                SIG_D_MSG_READ_MULTI,           // тип отправляемого сообщения
                m_message_factory,
                m_worker))
        {
          LOG(ERROR) << "Publish initial SBS update to '"<<identity<<"'";
          status_code = result.code();
        }

        // Освободить ресурсы info_all
        for (xdb::SubscriptionPoints_t::iterator it_info = points_list.begin();
             it_info != points_list.end();
             it_info++)
        {
          release_point_info(*it_info);
        }
        points_list.clear();
      }
  }

  return status_code;
}

// отправить адресату сообщение о статусе исполнения команды
// --------------------------------------------------------------------------------
void DiggerWorker::send_exec_result(int exec_val, std::string& identity)
{
  static std::string message_OK = "успешно";
  std::string message_FAIL;
  msg::ExecResult *msg_exec_result = 
        dynamic_cast<msg::ExecResult*>(m_message_factory->create(ADG_D_MSG_EXECRESULT));
  mdp::zmsg       *response = new mdp::zmsg();

  msg_exec_result->set_destination(identity);
  msg_exec_result->set_exec_result(exec_val);

  switch(exec_val)
  {
    case 0:
      msg_exec_result->set_failure_cause(0, message_OK);
    break;

    default:
      message_FAIL = xdb::Error::what(exec_val);
      msg_exec_result->set_failure_cause(exec_val, message_FAIL);
    break;
  }

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

  delete msg_exec_result;
#warning "replace send_to_broker(msg::zmsg*) to send(msg::Letter*)"
  // TODO: Для упрощения обработки сообщения не следует создавать zmsg и заполнять его
  // данными из Letter, а сразу напрямую отправлять Letter потребителю.
  // Возможно, следует указать тип передачи - прямой или через брокер.
  //1 send_to_broker((char*) MDPW_REPORT, NULL, response);
  response->send (m_worker); //1

  delete response;
}

DiggerPoller::DiggerPoller(zmq::context_t *ctx, xdb::RtEnvironment* env) :
 m_context(ctx),
 m_publisher(*m_context, ZMQ_PUB),
 m_environment(env),
 m_message_factory(new msg::MessageFactory(DIGGER_NAME))
{
  LOG(INFO) << "Create DiggerPoller, context " << m_context;
}

DiggerPoller::~DiggerPoller()
{
  LOG(INFO) << "start DiggerPoller destructor";
  delete m_message_factory;
  LOG(INFO) << "finish DiggerPoller destructor";
}

void DiggerPoller::work()
{
  xdb::RtConnection *db_connection = NULL;
  // Запрос на конфигурирование БДРВ
  xdb::rtDbCq operation;
  // Итератор групп подписки
  xdb::map_id_name_t sbs_map;
  // Итератор модифицированных точек
  xdb::map_id_name_t points_map;
  // Список модифицированных точек проверяемой группы
  std::vector<std::string> tags;
  // Итератор названий групп подписки
  xdb::map_id_name_t::iterator it_sbs;
  // Итератор модифицированных тегов атрибутов, участвующих в подписке
  xdb::map_id_name_t::iterator it_points;
  // Список элементарных записей чтения
  xdb::SubscriptionPoints_t points_list;
  // Набор опубликованных точек, для которых после нужно было скинуть флаг модификации 
  std::unordered_set <std::string> sbs_points_ready_to_clear_modif_flags;
  //1 std::vector<xdb::AttributeInfo_t*> info_all;
  bool result = false;
  // Код выполнения последнего запроса к БДРВ
  xdb::Error status;

#warning "Когда сбрасывать флаг модификации для точки, входящей в неск. SBS?"
// NB: Попробовать сбрасывать флаг модификации сразу для всех подобных точек,
// сразу после их успешной публикации подписчикам.

  try
  {
    LOG(INFO) << "DiggerPoller is ready to connect to subscriber's point '"
              << ENDPOINT_SBS_PUBLISHER << "'";
    m_publisher.bind("tcp://lo:5560" /* server form of ENDPOINT_SBS_PUBLISHER */);
    LOG(INFO) << "DiggerPoller connects to subscriber's point: " << ENDPOINT_SBS_PUBLISHER;

    db_connection = m_environment->getConnection();

    LOG(INFO) << "Start DiggerPoller::work, connection state="<<db_connection->state();
    while (!m_interrupt)
    {
      LOG(INFO) << "DiggerPoller ping";

      // Найти все группы, в которых изменились точки
      // ====================================================
      operation.action.query = xdb::rtQUERY_SBS_LIST_ARMED;
      operation.buffer = &sbs_map;
      status = db_connection->QueryDatabase(operation);
      if (status.Ok())
      {
        sbs_points_ready_to_clear_modif_flags.clear();

        // Пройтись по всем модифицированным Группам
        // ====================================================
        for (it_sbs = sbs_map.begin(); it_sbs != sbs_map.end(); it_sbs++)
        {
          LOG(INFO) << "Detect modified SBS " << it_sbs->first << ":'" << it_sbs->second << "'";

          // Читать все значения модифицированных атрибутов точек для указанной группы 
          operation.action.query = xdb::rtQUERY_SBS_READ_POINTS_ARMED;
          tags.clear();
          tags.push_back(it_sbs->second);
          operation.tags = &tags;
          // После выхода в points_list будут значения всех модифицированных
          // атрибутов точки с их именами.
          operation.buffer = &points_list;
          status = db_connection->QueryDatabase(operation);
          if (status.Ok())
          {
            LOG(INFO) << "rtQUERY_SBS_READ_POINTS_ARMED success";

            // Очередная публикация (название группы, перечень тегов со значениями)
            result = publish_sbs_impl(it_sbs->second,
                                 points_list,
                                 SIG_D_MSG_GRPSBS,
                                 m_message_factory,
                                 m_publisher);

            if (result)
            {
              // После публикации сбросить признак модификации успешно переданных точек
              // для указанной группы списка tags.
              // А пока собираем общий список переданных точек, чтобы всем им разом
              // снять признак модификации.
              for(xdb::SubscriptionPoints_t::iterator it = points_list.begin();
                  it != points_list.end();
                  it++)
              {
                sbs_points_ready_to_clear_modif_flags.insert((*it)->tag);
              }
            }

            // Освободить ресурсы info_all
            for (xdb::SubscriptionPoints_t::iterator it_info = points_list.begin();
                 it_info != points_list.end();
                 it_info++)
            {
              release_point_info(*it_info);
            }
            points_list.clear();
          }
        } // Конец цикла сбора информации о модифицированных точках

        // Сбросить признак модификации для успешно переданных точек
        operation.action.query = xdb::rtQUERY_SBS_POINTS_DISARM_BY_LIST;
        operation.buffer = &sbs_points_ready_to_clear_modif_flags;
        status = db_connection->QueryDatabase(operation);
        if (!status.Ok())
        {
          LOG(ERROR) << "Clear modification flags for published points, code="
                     << status.code() << " : " << status.what();
        }

      } // Конец успешного запроса списка активных групп с модифицированными точками

      usleep(PollingPeriod * 1000);
    }

    m_publisher.close();

    delete db_connection;
  }
  catch(zmq::error_t error)
  {
      LOG(ERROR) << "DiggerPoller catch: " << error.what();
  }
  catch (std::exception &e)
  {
      LOG(ERROR) << "DiggerPoller catch the signal: " << e.what();
  }

  LOG(INFO) << "Stop DiggerPoller::work";
  pthread_exit(NULL);
}

void DiggerPoller::stop()
{
  m_interrupt = true;
}

// После публикации сбросить признак модификации успешно переданных точек
// для указанной группы списка tags.
// NB: Одна и та же точка может участвовать в нескольких группах.
// TODO: как в таком случае сбрасывать флаг? Как определить, что эта группа
// обновления последняя?
bool DiggerPoller::clear_modification_flag(std::string& sbs_name, xdb::SubscriptionPoints_t& info_all)
{
  bool status = false;
  LOG(INFO) << "SBS clear_modification_flag: "<<info_all.size()<<" tags for '" << sbs_name << "'";
  return status;
}

void DiggerPoller::release_attribute_info(xdb::AttributeInfo_t* info)
{
  assert(info);

  switch(info->type)
  {
    case xdb::DB_TYPE_BYTES:
      delete info->value.dynamic.val_string;
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
      delete [] info->value.dynamic.varchar;
    break;
    default:
    // Остальные элементы структуры не являются динамически выделяемыми, пропустить
    ;
  }
}

// --------------------------------------------------------------------------------
DiggerProbe::DiggerProbe(zmq::context_t *ctx, xdb::RtEnvironment* env) :
 m_context(ctx),
 m_worker_command_socket(*m_context, ZMQ_PUB),
 m_environment(env),
 m_worker_list(),
 m_worker_thread(),
 m_sbs_checker(NULL),
 m_sbs_thread(NULL)
{
  LOG(INFO) << "DiggerProbe start, context " << m_context;
}

DiggerProbe::~DiggerProbe()
{
  LOG(INFO) << "start DiggerProbe destructor";
  LOG(INFO) << "finish DiggerProbe destructor";
}

bool DiggerProbe::start()
{
  bool status = false;

  if (kMaxWorkerThreads == start_workers())
  {
    // Запуск нити обработчика групп подписки
    status = start_sbs_poller();
  }

  return status;
}

// Создать экземпляры Обработчиков и запустить их нити
// Вернуть количество запущенных нитей
int DiggerProbe::start_workers()
{
  int linger = 0;
  int send_timeout_msec = SEND_TIMEOUT_MSEC; // 1 sec
  int recv_timeout_msec = RECV_TIMEOUT_MSEC; // 3 sec

  try
  {
    // Для inproc создать точку подключения к нитям Обработчиков ДО вызова connect в Клиентах
    m_worker_command_socket.bind(ENDPOINT_RTDB_COMMAND_BACKEND);
    m_worker_command_socket.setsockopt(ZMQ_LINGER, &linger, sizeof (linger));
    m_worker_command_socket.setsockopt(ZMQ_SNDTIMEO, &send_timeout_msec, sizeof(send_timeout_msec));
    m_worker_command_socket.setsockopt(ZMQ_RCVTIMEO, &recv_timeout_msec, sizeof(recv_timeout_msec));

    for (int i = 0; i < kMaxWorkerThreads; ++i)
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

// Создать экземпляр Управляющего группами подписки и запустить его нить
bool DiggerProbe::start_sbs_poller()
{
  // Создать объект DiggerPoller для управления Группами подписки
  LOG(INFO) << "DiggerProxy starting DiggerPoller thread";
  m_sbs_checker = new DiggerPoller(m_context, m_environment);
  m_sbs_thread = new std::thread(std::bind(&DiggerPoller::work, m_sbs_checker));
  return true;
}

void DiggerProbe::shutdown_sbs_poller()
{
  try
  {
    LOG(INFO) << "DiggerProbe start shuttingdown SBS poller";
    // Оповестить управляющего группами подписки о необходимости останова
    m_sbs_checker->stop();
    // TODO: корректно завершить работу
    usleep(100000);
    m_sbs_thread->join();
    delete m_sbs_checker;
    delete m_sbs_thread;
    LOG(INFO) << "DiggerProbe finish shuttingdown SBS poller";
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

// Остановить нити Обработчиков.
// Вызывается из DiggerProbe::work при значении true переменной m_interrupt.
// Значение m_interrupt меняется из DiggerProxy::work() при возврате из zmq_proxy
// при получении ей сигнала TERMINATE.
void DiggerProbe::shutdown_workers()
{
  // Количество оставшихся в работе нитей DiggerWorker, уменьшается от kMaxWorkerThreads до 0
  int threads_to_join;

  try
  {
    LOG(INFO) << "DiggerProbe start shuttingdown workers";
      // Вызвать останов функции DiggerWorker::work() для завершения треда
      for (int i = 0; i < kMaxWorkerThreads; ++i)
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
            if (time(0) > (when_joining_starts + 3))
            {
              LOG(ERROR) << "Timeout exceeds to joint DiggerProxy threads!";
            }
            LOG(INFO) << "Skip joining thread " << *it;
            usleep(100000);
          }
        }
      }
      
    LOG(INFO) << "DiggerProbe all workers joined";

      for(unsigned int i = 0; i < m_worker_list.size(); ++i)
      {
        LOG(INFO) << "delete DiggerWorker["<<i+1<<"/"<<m_worker_list.size()<<"] " << m_worker_list[i];
        delete m_worker_list[i];
      }

    LOG(INFO) << "DiggerProbe finish shuttingdown workers";
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
//  2. Установка флага m_interrupt в true
//
//  Необходимо опрашивать экземпляры DiggerWorker для:
//  1. Определения их работоспособности
//  2. Получения метрик обслуживания запросов
//  (min/avg/max, среднее время между запросами, ...)
//  --------------------------------------------------------------------------------
void DiggerProbe::work()
{
  xdb::RtConnection *db_connection = NULL;
  xdb::rtDbCq operation;
  xdb::Error result;

  // Проверить статус подключений
  operation.action.control = xdb::rtCONTROL_CHECK_CONN;

  try
  {
    db_connection = m_environment->getConnection();

    while (!m_interrupt)
    {
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

      result = db_connection->ControlDatabase(operation);
      LOG(INFO) << "Control database connections liveness: " << result.what();

      sleep(POLLING_PROBE_PERIOD);
    }

    LOG(INFO) << "DiggerProbe::work ready to shutdown";

    // Остановить Управляющего группами подписки
    shutdown_sbs_poller();

    // Послать сигнал TERMINATE Обработчикам и дождаться их завершения
    shutdown_workers();

//    usleep(500000);
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
  m_interrupt = true;
}

// --------------------------------------------------------------------------------
DiggerProxy::DiggerProxy(zmq::context_t *ctx, xdb::RtEnvironment* env) :
 m_context(ctx),
 m_control(*m_context, ZMQ_SUB),
 m_frontend(*m_context, ZMQ_ROUTER),
 m_backend(*m_context, ZMQ_DEALER),
 m_environment(env),
 m_probe(NULL),
 m_probe_thread(NULL)
{
  LOG(INFO) << "DiggerProxy start, context " << m_context;
}

// --------------------------------------------------------------------------------
//  Класс-прокси запросов от главной нити (Digger) к исполнителям (DiggerWorker)
// --------------------------------------------------------------------------------
DiggerProxy::~DiggerProxy()
{
  // NB: Ничего не делать, все ресурсы освобождаются при завершении run()
  LOG(INFO) << "start DiggerProxy destructor";
  LOG(INFO) << "finish DiggerProxy destructor";
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
  // Код ошибки zmq_proxy_steerable
  int rc = 1;
  int mandatory = 1;
  int linger = 0;
  int hwm = 100;
  int send_timeout_msec = SEND_TIMEOUT_MSEC; // 1 sec
  int recv_timeout_msec = RECV_TIMEOUT_MSEC; // 3 sec

  try
  {
    // Сокет прямого подключения к экземплярам DiggerWorker
    // NB: Выполняется в отдельной нити 
    //ZMQ_ROUTER_MANDATORY может привести zmq_proxy_steerable к аномальному завершению: rc=-1, errno=113
    //Наблюдалось в случаях интенсивного обмена брокера с клиентом, если последний аномально завершался.
    m_frontend.setsockopt(ZMQ_ROUTER_MANDATORY, &mandatory, sizeof (mandatory));
    m_frontend.bind("tcp://lo:5556" /*ENDPOINT_RTDB_FRONTEND*/);
    m_frontend.setsockopt(ZMQ_LINGER, &linger, sizeof (linger));
    m_frontend.setsockopt(ZMQ_RCVHWM, &hwm, sizeof(hwm));
    m_frontend.setsockopt(ZMQ_SNDHWM, &hwm, sizeof(hwm));
    m_frontend.setsockopt(ZMQ_SNDTIMEO, &send_timeout_msec, sizeof(send_timeout_msec));
    m_frontend.setsockopt(ZMQ_RCVTIMEO, &recv_timeout_msec, sizeof(recv_timeout_msec));
    LOG(INFO) << "DiggerProxy binds to DiggerWorkers frontend " << ENDPOINT_RTDB_FRONTEND;
    m_backend.bind(ENDPOINT_RTDB_DATA_BACKEND);
    m_backend.setsockopt(ZMQ_LINGER, &linger, sizeof (linger));
    m_backend.setsockopt(ZMQ_SNDTIMEO, &send_timeout_msec, sizeof(send_timeout_msec));
    m_backend.setsockopt(ZMQ_RCVTIMEO, &recv_timeout_msec, sizeof(recv_timeout_msec));
    LOG(INFO) << "DiggerProxy binds to DiggerWorkers backend " << ENDPOINT_RTDB_DATA_BACKEND;

    // Настройка управляющего сокета
    LOG(INFO) << "DiggerProxy is ready to connect with DiggerWorkers control";
    m_control.connect(ENDPOINT_RTDB_PROXY_CTRL);
    LOG(INFO) << "DiggerProxy connects to DiggerWorkers control " << ENDPOINT_RTDB_PROXY_CTRL;
    m_control.setsockopt(ZMQ_SUBSCRIBE, "", 0);
    m_control.setsockopt(ZMQ_LINGER, &linger, sizeof (linger));
    m_control.setsockopt(ZMQ_SNDTIMEO, &send_timeout_msec, sizeof(send_timeout_msec));
    m_control.setsockopt(ZMQ_RCVTIMEO, &recv_timeout_msec, sizeof(recv_timeout_msec));

    // Создать объект DiggerProbe для управления нитями DiggerWorker
    LOG(INFO) << "DiggerProxy starting DiggerProbe thread";
    m_probe = new DiggerProbe(m_context, m_environment);

    // Создать пул Обработчиков Службы и Управляющего группами подписки в DiggerProbe
    if (true == m_probe->start())
    {
      // Количество запущенных процессов совпадает с заданным.
      // Запустить нить управления DiggerProbe.
      m_probe_thread = new std::thread(std::bind(&DiggerProbe::work, m_probe));

      LOG(INFO) << "DiggerProxy (" << ENDPOINT_RTDB_FRONTEND
                << ", " << ENDPOINT_RTDB_DATA_BACKEND << ")";

      // NB: необходимо использовать оператор (void*) для доступа к внутреннему ptr
      // Перезапустить работу при завершении zmq_proxy_steerable с кодом EHOSTUNREACH=113,
      // это обычно связано с аварийным завершением Клиента в процессе обмена
      while (rc)
      {
        rc = zmq_proxy_steerable ((void*)m_frontend,
                                  (void*)m_backend,
                                  NULL /* C++11: nullptr */,
                                  (void*)m_control);
	    if (rc == EHOSTUNREACH) {
          LOG(WARNING) << "Restart DiggerProxy after EHOSTUNREACH";
          rc = 0;
        }
      }

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
      LOG(ERROR) << "Starting DiggerProbe: " << m_probe_thread;
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
  // Информировать Пробник о необходимости завершения его работы
  m_probe->stop();

  try
  {
    time_t when_joining_starts = time(0);
    // Дождаться останова
    LOG(INFO) << "Waiting joinable DiggerProbe thread";
    while (true)
    {
      if (m_probe_thread->joinable())
      {
        m_probe_thread->join();
        LOG(INFO) << "thread DiggerProbe joined";
        break;
      }
      else
      {
        // TODO: оформить время таймаута в нормальном виде (конфиг?)
        if (time(0) > (when_joining_starts + 3))
        {
          LOG(ERROR) << "Timeout exceeds to joint DiggerProxy Probe thread!";
        }
        usleep(500000);
        LOG(INFO) << "tick join DiggerProbe waiting thread";
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
Digger::Digger(const std::string& broker_endpoint, const std::string& service)
   :
   mdp::mdwrk(broker_endpoint, service, 1 /* num zmq io threads (default = 1) */, false /* direct messaging */),
   m_helpers_control(m_context, ZMQ_PUB),
   m_digger_proxy(NULL),
   m_proxy_thread(NULL),
   m_message_factory(new msg::MessageFactory(DIGGER_NAME)),
   m_appli(NULL),
   m_environment(NULL),
   m_db_connection(NULL)
{
  // Здесь используется m_context из mdp::mdwrk
  LOG(INFO) << "new Digger, context " << &m_context;
  m_appli = new xdb::RtApplication("DIGGER");
  // NB: Флаг OF_CREATE говорит о том, что текущий процесс будет являться владельцем БД,
  // то есть при завершении своей работы он может удалить экземпляр.
  m_appli->setOption("OF_CREATE",   1);    // Создать если БД не было ранее
  m_appli->setOption("OF_LOAD_SNAP",1);
  m_appli->setOption("OF_SAVE_SNAP",1);
  m_appli->setOption("OF_REGISTER_EVENT",1); // Регистрировать обработчики БДРВ
  // Максимальное число подключений к БД:
  // a) по одному на каждый экземпляр DiggerWorker (kMaxWorkerThreads)
  // b) один Управляющий Группами
  // c) один для Digger
  // d) один для мониторинга
  m_appli->setOption("OF_MAX_CONNECTIONS",  DiggerProbe::kMaxWorkerThreads + 10 + 1 + 1 + 1);
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
  LOG(INFO) << "start Digger destructor";

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
  LOG(INFO) << "finish Digger destructor";
}

// Запуск DiggerProxy и цикла получения сообщений
// --------------------------------------------------------------------------------
void Digger::run()
{
  interrupt_worker = false;

  try
  {
    m_environment = m_appli->loadEnvironment(RTDB_NAME);
    // Каждая нить процесса, желающая работать с БДРВ, должна получить свой экземпляр
    m_db_connection = m_environment->getConnection();

    LOG(INFO) << "RTDB status: " << m_environment->getLastError().what();

    LOG(INFO) << "DiggerProxy creating, interrupt_worker=" << (void*)&interrupt_worker;
    m_digger_proxy = new DiggerProxy(&m_context, m_environment);

    LOG(INFO) << "DiggerProxy starting Main thread";
    m_proxy_thread = new std::thread(std::bind(&DiggerProxy::run, m_digger_proxy));

    LOG(INFO) << "Wait a second to became DiggerProxy hot";
    sleep(1);

    LOG(INFO) << "DiggerProxy control connecting";
    m_helpers_control.bind("tcp://lo:5557" /*ENDPOINT_RTDB_PROXY_CTRL*/);

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

    LOG(INFO) << "Digger's message processing cycle is finished";
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
        usleep(500000);
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
  int rc = OK;
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
       rc = handle_asklife(letter, reply_to);
       break;

      default:
       LOG(ERROR) << "Unsupported request type: " << msgType;
       rc = NOK;
    }
  }
  else
  {
    LOG(ERROR) << "Readed letter "<<letter->header()->exchange_id()<<" not valid";
    rc = NOK;
  }

  delete letter;
  return rc;
}


// отправить адресату сообщение о статусе исполнения команды
// --------------------------------------------------------------------------------
void Digger::send_exec_result(int exec_val, std::string* reply_to)
{
  std::string s_OK = "хорошо";
  std::string s_FAIL = "плохо";
  msg::ExecResult *msg_exec_result = 
        dynamic_cast<msg::ExecResult*>(m_message_factory->create(ADG_D_MSG_EXECRESULT));
  mdp::zmsg       *response = new mdp::zmsg();

  msg_exec_result->set_destination(*reply_to);
  msg_exec_result->set_exec_result(exec_val);
  msg_exec_result->set_failure_cause(0, s_OK);

  response->push_front(const_cast<std::string&>(msg_exec_result->data()->get_serialized()));
  response->push_front(const_cast<std::string&>(msg_exec_result->header()->get_serialized()));
  response->wrap(reply_to->c_str(), "");

  LOG(INFO) << "Send ExecResult to " << *reply_to
            << " with status:" << msg_exec_result->exec_result()
            << " sid:" << msg_exec_result->header()->exchange_id()
            << " iid:" << msg_exec_result->header()->interest_id()
            << " dest:" << msg_exec_result->header()->proc_dest()
            << " origin:" << msg_exec_result->header()->proc_origin();

  delete msg_exec_result;
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
  int rc = OK;
  msg::AskLife   *msg_ask_life = static_cast<msg::AskLife*>(letter);
  mdp::zmsg      *response = new mdp::zmsg();
  int exec_val = 1;

  msg_ask_life->set_exec_result(exec_val);

  response->push_front(const_cast<std::string&>(msg_ask_life->data()->get_serialized()));
  response->push_front(const_cast<std::string&>(msg_ask_life->header()->get_serialized()));
  response->wrap(reply_to->c_str(), "");

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
      rc = NOK;
  }
#endif
  delete response;

  return rc;
}

// --------------------------------------------------------------------------------
#if !defined _FUNCTIONAL_TEST
int main(int argc, char **argv)
{
  int rc = OK;
  char given_service_name[SERVICE_NAME_MAXLEN + 1];
  std::string service_name;
  char given_broker_endpoint[ENDPOINT_MAXLEN + 1];
  std::string broker_endpoint = ENDPOINT_BROKER;
  bool is_service_name_given = false;
  int  opt;
  Digger *engine = NULL;

  ::google::InitGoogleLogging(argv[0]);
  ::google::InstallFailureSignalHandler();

  while ((opt = getopt (argc, argv, "b:s:")) != -1)
  {
     switch (opt)
     {
       case 'b': // точка подключения к Брокеру
         strncpy(given_broker_endpoint, optarg, ENDPOINT_MAXLEN);
         given_broker_endpoint[ENDPOINT_MAXLEN] = '\0';
         broker_endpoint.assign(given_broker_endpoint);
         break;

       case 's':
         strncpy(given_service_name, optarg, SERVICE_NAME_MAXLEN);
         given_service_name[SERVICE_NAME_MAXLEN] = '\0';
         service_name.assign(given_service_name);
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
         return EXIT_FAILURE;

       default:
         abort ();
     }
  }

  if (!is_service_name_given)
  {
    std::cout << "Service name not given.\nUse '-s <service>' [-b <broker endpoint>] options.\n";
    return EXIT_FAILURE;
  }

  try
  {
    engine = new Digger(broker_endpoint, service_name);

    LOG(INFO) << "Start service " << service_name;

    // Запуск нити DiggerProxy и нескольких DiggerWorker
    // ВЫполнение основного цикла работы приема-обработки-передачи сообщений
    engine->run();
  }
  catch(zmq::error_t err)
  {
    LOG(ERROR) << err.what();
    rc = NOK;
  }
  delete engine;
  LOG(INFO) << "Finish service " << service_name << ", rc=" << rc;

  ::google::protobuf::ShutdownProtobufLibrary();
  ::google::ShutdownGoogleLogging();

  return (OK == rc)? EXIT_SUCCESS : EXIT_FAILURE;
}
#endif

