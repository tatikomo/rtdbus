#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

// Общесистемные заголовочные файлы
#include <assert.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

// Служебные заголовочные файлы сторонних утилит
#include "glog/logging.h"

// Служебные файлы RTDBUS
#include "mdp_zmsg.hpp"
#include "mdp_worker_api.hpp"
#include "msg_adm.hpp"
#include "exchange_config.hpp"
#include "exchange_config_egsa.hpp"
#include "exchange_smad_int.hpp"
#include "exchange_smad_ext.hpp"
#include "exchange_egsa_impl.hpp"
#include "exchange_egsa_host.hpp"
#include "exchange_egsa_request_cyclic.hpp"
#include "xdb_common.hpp"

extern int interrupt_worker;

// ==========================================================================================================
EGSA_Host::EGSA_Host(std::string broker, std::string service, int verbose)
  : mdwrk(broker, service, 1),
    m_broker_endpoint(broker),
    m_server_name(service),
    m_egsa_instance(NULL),
    m_egsa_thread(NULL),
    m_message_factory(new msg::MessageFactory(EXCHANGE_NAME)),
    m_socket(-1)
{
}

// ==========================================================================================================
EGSA_Host::~EGSA_Host()
{
  delete m_message_factory;
}

// ==========================================================================================================
int EGSA_Host::init()
{
  int rc = OK;

  return rc;
}

// ==========================================================================================================
// Функция обрабатывает полученные служебные сообщения, не требующих подключения к БДРВ.
// Сейчас (2015.07.06) эта функция принимает запросы на доступ к БД, но не обрабатывает их.
int EGSA_Host::handle_request(mdp::zmsg* request, std::string* reply_to, bool& need_to_stop)
{
  int status = 0;
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
      case ADG_D_MSG_ASKLIFE:
        status = handle_asklife(letter, reply_to);
        break;

      case ADG_D_MSG_STOP:
        status = handle_stop(letter, reply_to);
        need_to_stop = true;
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
  return status;
}

// ==========================================================================================================
int EGSA_Host::handle_stop(msg::Letter* letter, std::string* reply_to)
{
  LOG(WARNING) << "Received message not yet supported received: ADG_D_MSG_STOP";
  interrupt_worker = 1;
  return OK;
}

// ==========================================================================================================
int EGSA_Host::handle_asklife(msg::Letter* letter, std::string* reply_to)
{
  msg::AskLife   *msg_ask_life = static_cast<msg::AskLife*>(letter);
  mdp::zmsg      *response = new mdp::zmsg();
  int exec_val = 1;

  msg_ask_life->set_exec_result(exec_val);

  response->push_front(const_cast<std::string&>(msg_ask_life->data()->get_serialized()));
  response->push_front(const_cast<std::string&>(msg_ask_life->header()->get_serialized()));
  response->wrap(reply_to->c_str(), EMPTY_FRAME);

  LOG(INFO) << "Processing asklife from " << *reply_to
            << " has status:" << msg_ask_life->exec_result(exec_val)
            << " sid:" << msg_ask_life->header()->exchange_id()
            << " iid:" << msg_ask_life->header()->interest_id()
            << " dest:" << msg_ask_life->header()->proc_dest()
            << " origin:" << msg_ask_life->header()->proc_origin();

  send_to_broker((char*) MDPW_REPORT, NULL, response);
  delete response;

  return OK;
}


// ==========================================================================================================
// 1. Дождаться сообщения ADDL о запросе готовности к работе
// 2. Прочитать конфигурационный файл
// 3. Инициализация подписки на атрибуты состояний Систем Сбора
// 4. Ответить на запрос готовности к работе
//    Если инициализация прошла неуспешно - выйти из программы
// 5. Дождаться сообщения ADDL о начале штатной работы
// 6. Цикл до получения сообщения об останове:
// 6.1. Проверять наступление очередного периода цикла опроса
// 6.2. Опрашивать пробуждения (?)
// 6.3. Опрашивать состояния СС
//
int EGSA_Host::run()
{
  int rc = OK;
  std::string *reply_to = NULL;
  // Устанавливается флаг в случае получения команды на "Останов" из-вне
  bool get_order_to_stop = false;
  const char* fname = "run()";

  LOG(INFO) << "Start " << fname;
  interrupt_worker = false;

  try {

    // Запустить обработчик запросов от Клиентов на получение истории
    m_egsa_instance = new EGSA(m_context);
    // Запустить Ответчика
    m_egsa_thread = new std::thread(std::bind(&EGSA::run, m_egsa_instance));

    if (OK == init()) {
      // TODO: Проверить состояние нити EGSA - как прочитался конфигурационный файл, подключение к SMAD, ...
      rc = OK;
    }

    while(!interrupt_worker)
    {
        reply_to = new std::string;

        LOG(INFO) << fname << " ready";
        // NB: Функция recv возвращает только PERSISTENT-сообщения
        mdp::zmsg *request = recv (reply_to);

        if (request)
        {
          LOG(INFO) << fname << " got a message";

          handle_request (request, reply_to, get_order_to_stop);

          if (get_order_to_stop) {
            LOG(INFO) << fname << " got a shutdown order";
            interrupt_worker = 1; // order to shutting down
          }

          delete request;
        }
        else
        {
          LOG(INFO) << fname << " got a NULL";
          interrupt_worker = 1; // has been interrupted
        }

        delete reply_to;
    }
  }
  catch(zmq::error_t error)
  {
    interrupt_worker = true;
    rc = NOK;
    LOG(ERROR) << error.what();
  }
  catch(std::exception &e)
  {
    interrupt_worker = true;
    rc = NOK;
    LOG(ERROR) << e.what();
  }

  // Послать в HistorianResponder сигнал завершения работы
  m_egsa_instance->stop();
  m_egsa_thread->join();
  delete m_egsa_thread;
  delete m_egsa_instance;

  LOG(INFO) << "Finish " << fname;

  return rc;
}

// ==========================================================================================================
