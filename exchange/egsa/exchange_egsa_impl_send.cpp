#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

// Общесистемные заголовочные файлы
#include <assert.h>

// Служебные заголовочные файлы сторонних утилит
#include "glog/logging.h"

// Служебные файлы RTDBUS
#include "exchange_egsa_site.hpp"
#include "exchange_egsa_impl.hpp"
#include "exchange_egsa_request.hpp"

extern int interrupt_worker;

// ==========================================================================================================
// Обменивается данными только с верхним уровнем EGSA::implementation
int EGSA::implementation_send()
{
  static const char *fname = "EGSA::implementation_send";
  bool need_to_stop = false;
  bool changed = true;
  int linger = 0;
  int send_timeout_msec = SEND_TIMEOUT_MSEC;
  int recv_timeout_msec = RECV_TIMEOUT_MSEC;

  zmq::pollitem_t socket_items[2];
  // Сокет для приема данных от смежных систем.
  zmq::socket_t es_send(m_context, ZMQ_PUSH);
  const int PollingTimeout = 1000;
  int rc = OK;

#ifdef _FUNCTIONAL_TEST
  LOG(INFO) << fname << ": will use FUNCTIONAL_TEST";
#endif

  try
  {
    // Сокет для обмена командами с нитью EGSA::implementation
    m_send_control = new zmq::socket_t (m_context, ZMQ_PAIR);

    m_send_control->bind(ENDPOINT_EGSA_SEND_BACKEND);
    m_send_control->setsockopt(ZMQ_LINGER, &linger, sizeof(linger));
    m_send_control->setsockopt(ZMQ_SNDTIMEO, &send_timeout_msec, sizeof(send_timeout_msec));
    m_send_control->setsockopt(ZMQ_RCVTIMEO, &recv_timeout_msec, sizeof(recv_timeout_msec));

    socket_items[0].socket = (void *) *m_send_control;  // NB: (void*) обязателен
    socket_items[0].fd = 0;
    socket_items[0].events = ZMQ_POLLIN;
    socket_items[0].revents = 0;

    socket_items[1].socket = (void *) es_send;  // NB: (void*) обязателен
    socket_items[1].fd = 0;
    socket_items[1].events = ZMQ_POLLIN;
    socket_items[1].revents = 0;

    init_send();

    while (!interrupt_worker && (send_state()!=STATE_STOP))
    {
      LOG(INFO) << fname << ": current state " << send_state();

      zmq::poll(socket_items, 2, PollingTimeout);

      if (socket_items[0].revents & ZMQ_POLLIN)         // <==================== Запрос от EGSA::implementation
      {  
        mdp::zmsg * request = new mdp::zmsg(*m_send_control);
        assert(request);
        process_send_command(request);

        delete request;
      }
      else if (socket_items[1].revents & ZMQ_POLLIN)    // <==================== Запрос от внешней системы
      {
        mdp::zmsg * request = new mdp::zmsg(es_send);
        assert(request);

        // replay address
        const std::string identity  = request->pop_front();
        const std::string empty = request->pop_front();

        rc = process_send_request(request, identity);
        delete request;
      } // Запрос от смежных систем

    } // конец цикла приема и обработки сообщений

    m_send_control->close();
    delete m_send_control;
    es_send.close();

  } // конец блока try-catch
  catch(zmq::error_t error)
  {
    LOG(ERROR) << fname << ": catch: " << error.what();
    rc = NOK;
  }
  catch(std::exception & e)
  {
    LOG(ERROR) << fname << ": catch the signal: " << e.what();
    rc = NOK;
  }

  fini_send();

  LOG(INFO) << fname << ": done";

  pthread_exit(NULL);

  return rc;
}

// ==========================================================================================================
int EGSA::init_send()
{
  int rc = OK;

  LOG(INFO) << "Start ES_SEND thread";
  
  // TODO: подключиться к XDB

  change_send_state_to(STATE_INITIAL);

  return rc;
}

// ==========================================================================================================
int EGSA::fini_send()
{
  int rc = OK;

  LOG(INFO) << "Finish ES_SEND thread";
  // TODO: отключиться от БД
  return rc;
}

// ==========================================================================================================
int EGSA::process_send_command(mdp::zmsg* request)
{
  static const char *fname = "EGSA::process_send_command";
  const char* report = internal_report_string[UNWILLING];
  const std::string command_identity = request->pop_front();
  int rc = NOK;

  if (0 == command_identity.compare(TK_MESSAGE))        // <<<========================== Циклическое сообщение
  {
    // CASE input type is waken-up to delete old requests
    const std::string cycle_identity = request->pop_front();
    LOG(INFO) << fname << ": timekeeper message " << cycle_identity;
    if (0 == cycle_identity.compare(OLDREQUEST)) {
      //rc = esg_acq_rsh_ReqsDelMan();
    }
    else if (0 == cycle_identity.compare(SENDAGAIN)) {
      //rc = esg_acq_rsh_ReqsDelMan();
    }
  }
  else if (0 == command_identity.compare(ASKLIFE))      // <<========================== Запрос состояния
  {
    report = internal_report_string[GOOD];
    rc = OK;
  }
  else if (0 == command_identity.compare(INIT))         // <<========================== Приказ о начале инициализации
  {
    if (send_state() == STATE_INITIAL) { // Процесс ждал инициализацию?
      // Выполнить подготовительные работы
      //
      // This routine gets the address to access to the diffused data from each acquisition site (memory address)
      if (OK == (rc = esg_ine_ini_OpenDifDataAccess())) 
      {
        // Memory allocation for Exchanged infos of all acquisition sites
        rc = esg_ine_ini_AllocExchInfo("ESG_INE_CNF_D_DIFFMODE");
      }

      if (OK == rc)
      {
        report = internal_report_string[GOOD];
        change_send_state_to(STATE_INI_OK);
        // ....
        change_send_state_to(STATE_WAITING_INIT);
      }
	  else
      {
        report = internal_report_string[FAIL];
        change_send_state_to(STATE_INI_KO);
      }

    }
    else { // Не ожидалось получение команды об инициализации
      report = internal_report_string[ALREADY];
      LOG(WARNING) << fname << ": unwilling INIT message in state " << send_state();
    }
  }
  else if (0 == command_identity.compare(ENDALLINIT))   // <========================== Все соседние процессы инициализировались
  {
    if (send_state() == STATE_WAITING_INIT) // Процесс ждал завершения инициализации?
    { 
      report = internal_report_string[GOOD];
      change_send_state_to(STATE_END_INIT);

      // Поделаем чего-нибудь полезного...
      esg_esg_odm_TraceAllCycleEntry ();

      // ask wakeups
      // ---------------------------------------------------------------
      // Delete sites in diffusion cycles without primary/secondary data
      // ---------------------------------------------------------------
      rc = esg_ine_ini_UpdateDataDiffCycle ();

      // ---------------------------------------------------------------
      //  ask for cyclic wakeups according to the acquisition cycles and the periods defined in configuration
      // ---------------------------------------------------------------
      rc = esg_ine_tim_WakeupsAsk ();

#if 0
      // ---------------------------------------------------------------
      // Re-start the emergency diffusion cycle if previously on emergency state 
      // The process might be stopped during emergency state
      // --------------------------------------------------------
      rc = esg_snd_inm_EmergenCycStart();
#endif

      // ---------------------------------------------------------------
      // Ask AS states
      // ---------------------
      rc = esg_ine_ini_GetStatesMode ();

      // ---------------------------------------------------------------
      // alarms and history subscriptions
      // ---------------------------------------
      rc = esg_ine_ini_AlaHistSub ();

      if (OK == rc)
      {
        report = internal_report_string[GOOD];
        change_send_state_to(STATE_RUN); // Переход в рабочее состояние
      }
      else
      {
        report = internal_report_string[FAIL];
      }
    }
    else { // Не ожидалось получение команды о завершении инициализации
      report = internal_report_string[ALREADY];
      LOG(WARNING) << fname << ": unwilling ENDALLINIT message in state " << send_state();
    }
  }
  else if (0 == command_identity.compare(SHUTTINGDOWN)) // <<========================== Приказ о завершении работы
  {
      report = internal_report_string[GOOD];
      change_send_state_to(STATE_SHUTTINGDOWN);
      change_send_state_to(STATE_STOP);
  }
  else                                                  // <=========================== Необслуживаемая (еще?) команда
  {
      report = internal_report_string[UNWILLING];
      LOG(ERROR) << fname << ": get unsupported command: " << command_identity;
      rc = NOK;
  }

  // Ответить на сообщение о завершении инициализации
  m_send_control->send(report, strlen(report));

  LOG(INFO) << fname << ": run";
  return rc;
}

// ==========================================================================================================
// gets the address to access to the diffused data from each acquisition site
int EGSA::esg_ine_ini_OpenDifDataAccess()
{
  static const char* fname = "esg_ine_ini_OpenDifDataAccess";
  int rc = OK;

  LOG(INFO) << fname << ": run - gets the address to access to the diffused data from each acquisition site";
  return rc;
}

// ==========================================================================================================
// Memory allocation for Exchanged infos of all acquisition sites
int EGSA::esg_ine_ini_AllocExchInfo(const char*)
{
  static const char* fname = "esg_ine_ini_AllocExchInfo";
  int rc = OK;

  LOG(INFO) << fname << ": run";
  return rc;
}

// ==========================================================================================================
int EGSA::esg_ine_tim_WakeupsAsk()
{
  static const char* fname = "esg_ine_tim_WakeupsAsk";
  int rc = OK;

  LOG(INFO) << fname << ": run";
  return rc;
}

// ==========================================================================================================
int EGSA::esg_esg_odm_TraceAllCycleEntry()
{
  static const char* fname = "esg_esg_odm_TraceAllCycleEntry";
  int rc = OK;

  LOG(INFO) << fname << ": run";
  return rc;
}

// ==========================================================================================================
int EGSA::esg_ine_ini_UpdateDataDiffCycle()
{
  static const char* fname = "esg_ine_ini_UpdateDataDiffCycle";
  int rc = OK;

  LOG(INFO) << fname << ": run";
  return rc;
}

// ==========================================================================================================
int EGSA::esg_ine_ini_GetStatesMode()
{
  static const char* fname = "esg_ine_ini_GetStatesMode";
  int rc = OK;

  LOG(INFO) << fname << ": run";
  return rc;
}

// ==========================================================================================================
int EGSA::esg_ine_ini_AlaHistSub()
{
  static const char* fname = "esg_ine_ini_AlaHistSub";
  int rc = OK;

  LOG(INFO) << fname << ": run";
  return rc;
}

// ==========================================================================================================
// Обработка запросов от смежных Систем
int EGSA::process_send_request(mdp::zmsg* request, const std::string& identity)
{
  static const char* fname = "EGSA::process_send_request";
  int rc = NOK;
  rtdbMsgType msgType;
  rtdbExchangeId message_type;

  msg::Letter *letter = m_message_factory->create(request);
  if (letter->valid())
  {
    msgType = letter->header()->usr_msg_type();
    LOG(INFO) << fname << ": Receive type:" <<  msgType;

    switch (msgType)
    {
      default:
        LOG(ERROR) << fname << ": ESG_ESG_D_ERR_UNKNOWNMESS: " << msgType;
        break;
    } // end switch message type

  } // end message is valid

  return rc;
}
// ==========================================================================================================

// ==========================================================================================================
