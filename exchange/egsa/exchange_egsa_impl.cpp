#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

// Общесистемные заголовочные файлы
#include <assert.h>
#include <sys/time.h>   // for 'time_t' and 'struct timeval'
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>     // for 'usleep'
#include <iostream>
#include <fstream>
#include <functional>
#include <queue>
#include <chrono>
#include <thread>

// Служебные заголовочные файлы сторонних утилит
#include "glog/logging.h"

// Служебные файлы RTDBUS
#include "tool_events.hpp"
#include "mdp_common.h"
#include "mdp_zmsg.hpp"
#include "mdp_worker_api.hpp"
#include "msg_common.h"     // константы запросов SIG_D_MSG_*
// сообщения общего порядка
#include "msg_adm.hpp"
// сообщения по работе с БДРВ
#include "msg_sinf.hpp"
#include "exchange_config.hpp"
#include "exchange_config_egsa.hpp"
//#include "exchange_config_site.hpp"
#include "exchange_config_request.hpp"
#include "exchange_smad.hpp"
#include "exchange_smed.hpp"
#include "exchange_egsa_impl.hpp"
#include "exchange_egsa_translator.hpp"
#include "xdb_common.hpp"

extern volatile int interrupt_worker;
const char* EGSA::internal_report_string[] = { "GOOD", "ALREADY", "FAIL", "UNWILLING" };
static const char* SYNTHSTATE_LABELS[] = {
      /* 0 */ "UNREACHABLE",    /* SYNTHSTATE_UNREACH */
      /* 1 */ "OPERATIVE",      /* SYNTHSTATE_OPER */
      /* 2 */ "INITIALIZATION", /* SYNTHSTATE_PRE_OPER */
      /* 3 */ "UNKNOWN" };
typedef enum {
    GOF_D_FAILURE = 0,
    GOF_D_SUCCESS = 1
} gof_state_t;

// ==========================================================================================================
EGSA::EGSA(const std::string& _broker, const std::string& _service)
  : mdp::mdwrk(_broker, _service, 1 /* num threads */, true /* use  direct connection */),
    m_state(STATE_INITIAL),
    m_state_egsa(STATE_INITIAL),
    m_state_acq(STATE_INITIAL),
    m_state_send(STATE_INITIAL),
    m_message_factory(new msg::MessageFactory(EXCHANGE_NAME)),
    m_backend_socket(m_context, ZMQ_PAIR), // Сокет обмена сообщениями с Интерфейсом второго уровня EGSA::implementation
    m_acq_control(NULL),    // Создается в нити ES_ACQ
    m_send_control(NULL),   // Создается в нити ES_SEND
    m_servant_thread(NULL),
    m_acquisition_thread(NULL),
    m_sending_thread(NULL),
    m_smed(NULL),
    m_egsa_config(NULL),
    m_translator(NULL)
{
}

// ==========================================================================================================
EGSA::~EGSA()
{
  // Отключиться от SMAD Сайтов
  detach();
  // НСИ сведения о Запросах не хранятся в БД
  //1 m_ega_ega_odm_ar_Requests.release();

  m_backend_socket.close();
  // Эти сокеты удаляются в функциях их создания
  //delete m_acq_control;
  //delete m_send_control;

  delete m_translator;
  delete m_smed;

#ifdef _FUNCTIONAL_TEST
  // Для тестов нужно было держать конфигурацию доступной до конца работы,
  // но в рабочей программе этот объект можно удалять сразу после получения из него данных.
  delete m_egsa_config;
#endif

  delete m_message_factory;
}

// ==========================================================================================================
int EGSA::init()
{
  const char* fname = "EGSA::init";
  int status = OK;
  int linger = 0;
  //int hwm = 100;
  int send_timeout_msec = SEND_TIMEOUT_MSEC; // 1 sec
  int recv_timeout_msec = RECV_TIMEOUT_MSEC; // 3 sec
  smad_connection_state_t ext_state;

  // Инициализировать массивы Сайтов и Циклов
  load_config();
  LOG(INFO) << fname << ": configs loaded, state=" << m_state;

  // Подключиться к своей внутренней памяти SMED, передав в качестве начальных значений объект конфигурации
  m_smed = new SMED(m_egsa_config, m_smed_filename.c_str());
  LOG(INFO) << fname << ": SMED created, state=" << m_smed->state();

  if (STATE_OK == (ext_state = m_smed->connect())) {

#if 0
    Запросы НСИ загружаются в момент подключения к неинициализированной SMED
    // Загрузить НСИ Запросов
    if (OK != (status = store_requests_dict())) {
      LOG(ERROR) << fname << ": Storing Requests into SMED : rc=" << status;
    }
#endif

    // Загрузить словари обменов со смежными Сайтами
    if (OK == status) {
      if (OK == (status = load_all_dictionaries())) {
#ifndef _FUNCTIONAL_TEST
        // Активировать группу подписки
        status = activateSBS();
        LOG(INFO) << fname << ": SBS activated";
#else
        LOG(WARNING) << fname << ": FUNCTIONAL_TEST: skip SMED and SBS facilities";
#endif
      }
    }

    if (OK == status) {
      try {
        // Для inproc создать точку подключения (bind) к нитям Обработчиков ДО подключения (connect) в Клиентах
        m_backend_socket.bind(ENDPOINT_EGSA_BACKEND);
        LOG(INFO) << fname << ": bind EGSA backend " << ENDPOINT_EGSA_BACKEND;
        m_backend_socket.setsockopt(ZMQ_LINGER, &linger, sizeof (linger));
        m_backend_socket.setsockopt(ZMQ_SNDTIMEO, &send_timeout_msec, sizeof(send_timeout_msec));
        m_backend_socket.setsockopt(ZMQ_RCVTIMEO, &recv_timeout_msec, sizeof(recv_timeout_msec));

        m_state = STATE_INI_OK;
      }
      catch(zmq::error_t error)
      {
        LOG(ERROR) << fname << ": catch: " << error.what();
        status = NOK;
      }
      catch (std::exception &e)
      {
        LOG(ERROR) << fname << ": catch the signal: " << e.what();
        status = NOK;
      }
    } // end if SBS activated normally
  } // end if connected to SMED normally
  else {
    LOG(ERROR) << "Connecting to SMED, code=" << ext_state;
  }

  if (NOK == status)
    m_state = STATE_INI_KO;

  return status;
}

#if 0
// ==========================================================================================================
// Загрузка словаря Запросов в SMED
int EGSA::store_requests_dict()
{
  const char *fname = "store_requests_dict";
  int rc = OK;

  assert(m_smed);
  assert(m_smed->state() == STATE_OK);

  LOG(INFO) << fname << ": RUN";

  for(int id= 0; id < 80; id++) {
    Request *r = m_ega_ega_odm_ar_Requests[static_cast<ech_t_ReqId>(id)];
    if (r)
      LOG(INFO) << fname << ": Req " << r->name() << " ";
  }

  return rc;
}
#endif

// ==========================================================================================================
// Загрузка указанного обменного словаря
int EGSA::load_dict(const char* sa_dict_file)
{
  int rc = OK;

  rc = m_smed->load_dict(sa_dict_file);
  LOG(INFO) << "Loading ESG dictionary " << sa_dict_file << " into SMED, rc=" << rc;

  return rc;
}

// ==========================================================================================================
// Загрузка обменных словарей для известных Сайтов
// Прочитать набор словарей для своего объекта. Тип режима закодирован в названии словаря.
// Формат имени файла: {SND|ACQ}INFOS.<Код удаленного объекта>.json
// (SND)DIFFUSION - что локальный объект должен отправить
// ACQUISITION - что локальный объект хочет получить
int EGSA::load_all_dictionaries()
{
  int rc = NOK;
  char dict_filename[255];
  SmedObjectList<AcqSiteEntry*>* all_sa = smed()->SiteList();

  for(std::size_t i=0; i < all_sa->count(); i++) {

    AcqSiteEntry* site = (*all_sa)[i];

    if (site) {
      sprintf(dict_filename, "SNDINFOS.%s.json", site->name());
      //
      LOG(INFO) << "LOAD " << dict_filename;
      rc = load_dict(dict_filename);
      if (OK != rc) {
        // Не удалось загрузить словарь обмена с Сайтом
        // TODO: Исключить его из опроса? Или ограничиться минимальным набором данных (только свое состояние)
        LOG(ERROR) << "TODO: Exclude site " << site->name() << " due its loading dictionary error";
      }
      LOG(INFO) << site->name() << ": SND Dictionary " << dict_filename << " loading status=" << rc;
    }

  }

  return rc;
}

// ==========================================================================================================
// На входе информация из конфигурационного файла. Она может содержать неотсортированные данные.
// То есть m_egsa_config->load() выполняет первый проход по обработке информации - проверяет ее на
// корректность и загружает все в символьном виде (названия Циклов, Сайтов и типов Запросов)
// Второй проход обработки заключается в переходе от символьного представления НСИ к идентификаторам
// и ссылкам, для ускорения работы
int EGSA::load_config()
{
  const char* fname = "EGSA::load_config";
  ech_t_ReqId request_id;
  int rc = OK;

  // Была ли ранее уже загружена конфигурация?
  if (m_egsa_config) {
    // Была - очистим старые данные и перечитаем её
    LOG(INFO) << fname << ": reload configuration, state=" << m_state;

    delete m_translator;
    delete m_egsa_config;
  }

  // Открыть конфигурацию
  m_egsa_config = new EgsaConfig("egsa.json");

  // Прочитать информацию по сайтам и циклам
  m_egsa_config->load();
  m_smed_filename = m_egsa_config->smed_name();
  m_state = STATE_CNF_OK;
  // Сейчас загружены списки актуальных Циклов, Запросов, Сайтов:
  // m_egsa_config->sites()
  // m_egsa_config->cycles()
  // m_egsa_config->requests()

  // Создать словари ESG
  m_translator = new ExchangeTranslator(this,
                                        config()->elemtypes(),
                                        config()->elemstructs());

#ifndef _FUNCTIONAL_TEST
  // Если это не тест, объект конфигурации более не нужен
  delete m_egsa_config;
  m_egsa_config = NULL;
#endif

  return rc;
}

// ==========================================================================================================
// Доступ к конфигурации
EgsaConfig* EGSA::config()
{
#ifndef _FUNCTIONAL_TEST
  LOG(FATAL) << "Access to configuration class instance";
#endif
  return m_egsa_config;
}

// ==========================================================================================================
// Подключиться к SMAD систем сбора
// TEST - подключиться к SMAD для каждой подчиненной системы
int EGSA::attach_to_sites_smad()
{
  int rc = OK;

  // По списку известных нам систем сбора создать интерфейсы к их SMAD
  for (std::size_t i=0; i < sites()->count(); i++) {

#warning "Нужно предусмотреть возможность работы системы сбора и EGSA на разных хостах"
    // TODO: СС и EGSA могут работать на разных хостах, в этом случае подключение EGSA к smad СС
    // не получит доступа к реальным данным от СС. Их придется EGSA туда заносить самостоятельно.
    rc |= (*sites())[i]->attach_smad();
  }

  return OK;
}

// ==========================================================================================================
// Изменение состояния подключенных систем сбора и отключение от их внутренней SMAD
int EGSA::detach()
{
  int rc = OK;

  // TODO: Для всех подчиненных систем сбора:
  // 1. Изменить их состояние SYNTHSTATE на "ОТКЛЮЧЕНО"
  // 2. Отключиться от их внутренней SMAD
  for (size_t i=0; i < sites()->count(); i++) {

    LOG(INFO) << "TODO: set " << (*sites())[i]->name() << "." << RTDB_ATT_SYNTHSTATE << " = 0";
    LOG(INFO) << "TODO: detach " << (*sites())[i]->name() << " SMAD";
    rc |= (*sites())[i]->detach_smad();

  }

  return rc;
}

// ==========================================================================================================
// Обработка команд от EGSA::run()
int EGSA::process_command(const std::string& command)
{
  const char *fname = "EGSA::process_command";
  int rc = NOK;

  if (0 == command.compare("TERMINATE")) {
    LOG(INFO) << fname << ": process " << command;
  }
  else if (0 == command.compare("PROBE")) {
    LOG(INFO) << fname << ": process " << command;
  }
  else {
    LOG(ERROR) << fname << ": unsupported command: " << command;
  }

  return rc;
}

// ==========================================================================================================
// Интерфейс реализации второго уровня
// Отвечает за взаимодействие с интерфейсными модулями систем сбора и смежными объектами
int EGSA::implementation()
{
  const char* fname = "EGSA::implementation";
  int status = OK;
  bool need_to_stop = false;
  bool changed = true;
  mdp::zmsg *request = NULL;
  const int PollingTimeout = 1000;
  int linger = 0;
  int send_timeout_msec = SEND_TIMEOUT_MSEC;
  int recv_timeout_msec = RECV_TIMEOUT_MSEC;
  zmq::pollitem_t  socket_items[3];
  zmq::socket_t  commands(m_context, ZMQ_PAIR);
  zmq::socket_t  es_acq(m_context, ZMQ_PAIR);
  zmq::socket_t  es_send(m_context, ZMQ_PAIR);

#ifdef _FUNCTIONAL_TEST
  LOG(INFO) << fname << ": will use FUNCTIONAL_TEST";
#endif

  try {
    // Подключиться к уже существующему сокету управляющей нитки
    commands.connect(ENDPOINT_EGSA_BACKEND);
    LOG(INFO) << fname << ": connects to EGSA frontend " << ENDPOINT_EGSA_BACKEND;
    commands.setsockopt(ZMQ_LINGER, &linger, sizeof (linger));
    commands.setsockopt(ZMQ_SNDTIMEO, &send_timeout_msec, sizeof(send_timeout_msec));
    commands.setsockopt(ZMQ_RCVTIMEO, &recv_timeout_msec, sizeof(recv_timeout_msec));

    // Создать подключение к нитке опроса смежных систем
    es_acq.bind(ENDPOINT_EGSA_ACQ_BACKEND);
    LOG(INFO) << fname << ": bind to EG_ACQ backend " << ENDPOINT_EGSA_ACQ_BACKEND;
    // Создать подключение к нитке отправки данных в адрес смежных систем
    es_send.bind(ENDPOINT_EGSA_SEND_BACKEND);
    LOG(INFO) << fname << ": bind to EG_SEND backend " << ENDPOINT_EGSA_SEND_BACKEND;

    socket_items[0].socket = (void*)commands; // NB: (void*) обязателен
    socket_items[0].fd = 0;
    socket_items[0].events = ZMQ_POLLIN;
    socket_items[0].revents = 0;

    socket_items[1].socket = (void*)es_acq; // NB: (void*) обязателен
    socket_items[1].fd = 0;
    socket_items[1].events = ZMQ_POLLIN;
    socket_items[1].revents = 0;

    socket_items[2].socket = (void*)es_send; // NB: (void*) обязателен
    socket_items[2].fd = 0;
    socket_items[2].events = ZMQ_POLLIN;
    socket_items[2].revents = 0;

#ifdef _FUNCTIONAL_TEST
    const time_t when_starts = time(0);
#endif

    // Запомним как первоначальное значение
    //last_probe_time = cur_time = time(0);

    // Высший приоритет останова у интерфейса Службы (EGSA::run получил сигнал останова)
    // Низший приоритет останова у нити implementation (по получении команды STOP)
    while(!interrupt_worker && !need_to_stop) {
      LOG(INFO) << fname;

      zmq::poll (socket_items, 3, PollingTimeout);

      if (socket_items[0].revents & ZMQ_POLLIN) { // Получен запрос верхнего уровня
          request = new mdp::zmsg (commands);
          assert (request);
          // command or replay address
          std::string command_identity  = request->pop_front();
          process_command(command_identity);
          delete request;
      } // конец блока обработки запроса верхнего уровня
      else if (socket_items[1].revents & ZMQ_POLLIN) { // Получен запрос от ES_ACQ
          request = new mdp::zmsg (es_acq);
          assert (request);
          std::string command_identity  = request->pop_front();
          LOG(INFO) << fname << ": get es_acq replay:" << command_identity;
          process_acq(command_identity);
          delete request;
      }
      else if (socket_items[2].revents & ZMQ_POLLIN) { // Получен запрос от ES_SEND
          request = new mdp::zmsg (es_send);
          assert (request);
          std::string command_identity  = request->pop_front();
          LOG(INFO) << fname << ": get es_send replay:" << command_identity;
          process_send(command_identity);
          delete request;
      }
      else {
        LOG(INFO) << fname << ": idle";
      }

#ifdef _FUNCTIONAL_TEST
      const time_t now = time(0);
#endif

      switch(m_state_egsa) {
        // Инициализация нити
        // ------------------
        case STATE_INITIAL:
          LOG(INFO) << fname << ": state INITIAL";
#ifdef _FUNCTIONAL_TEST
          if ((now - when_starts) == 1)
#endif
          {
            es_acq.send(INIT, strlen(INIT));
            es_send.send(INIT, strlen(INIT));
            LOG(INFO) << fname << ": sent request " << INIT << " to ES";
            changed = change_egsa_state_to(STATE_WAITING_INIT);
          }
          break;

        // Ожидание разрешения на работу
        // -----------------------------------
        case STATE_WAITING_INIT:
          LOG(INFO) << fname << ": state WAITING_INIT";
          // Ждем от Брокера сигнала о завершении инициализации - начале работ
#ifdef _FUNCTIONAL_TEST
          if ((now - when_starts) == 3)
#endif
          {
            changed = change_egsa_state_to(STATE_END_INIT);
          }

        // Получили сигнал о завершении инициализации, можно работать
        // ----------------------------------------------------------
        case STATE_END_INIT:
          LOG(INFO) << fname << ": state END_INIT";
#ifdef _FUNCTIONAL_TEST
          if ((now - when_starts) == 4)
#endif
          {
            changed = change_egsa_state_to(STATE_RUN);
            es_acq.send(ENDALLINIT, strlen(ENDALLINIT));
            es_send.send(ENDALLINIT, strlen(ENDALLINIT));
            LOG(INFO) << fname << ": sent request " << ENDALLINIT << " to ES";
            activate_cycles();
          }
          break;

        // Получили сигнал о завершении работы, останавливаем нить
        // ----------------------------------------------------------
        case STATE_SHUTTINGDOWN:
          LOG(INFO) << fname << ": state SHUTTINGDOWN";
#ifdef _FUNCTIONAL_TEST
          if ((now - when_starts) == 11)
#endif
          {
            changed = change_egsa_state_to(STATE_SHUTTINGDOWN);
            es_acq.send(STOP, strlen(SHUTTINGDOWN));
            es_send.send(STOP, strlen(SHUTTINGDOWN));
            LOG(INFO) << fname << ": sent request " << SHUTTINGDOWN << " to ES";
            deactivate_cycles();
          }
          interrupt_worker = true;
          need_to_stop = true;
          changed = change_egsa_state_to(STATE_STOP);
          break;

        // Оперативный режим, ждем команд управления и опроса
        // --------------------------------------------------
        case STATE_RUN:
          LOG(INFO) << fname << ": state RUN";
#ifdef _FUNCTIONAL_TEST
          if ((now - when_starts) == 10)
#endif
          {
            changed = change_egsa_state_to(STATE_STOP);
          }
          break;

        case STATE_STOP:
          LOG(INFO) << fname << ": state STOP";
          need_to_stop = true;
          interrupt_worker = true;
          break;

        default:
          LOG(FATAL) << fname << ": unhandled state #" << m_state_egsa;
      } // end switch

/*
      // Ежесекундный опрос состояния нитей
      es_acq.send(ASKLIFE, strlen(ASKLIFE));
      es_send.send(ASKLIFE, strlen(ASKLIFE));
      LOG(INFO) << fname << ": sent request " << ASKLIFE << " to ES";
*/

      cycles::timer();

      process_requests_for_all_sites();

    }  // конец цикла while

    es_acq.close();
    es_send.close();

  } // конец блока try-catch
  catch(zmq::error_t error)
  {
    LOG(ERROR) << fname << ": catch: " << error.what();
    status = NOK;
  }
  catch (std::exception &e)
  {
    LOG(ERROR) << fname << ": catch the signal: " << e.what();
    status = NOK;
  }

  commands.close();

  pthread_exit(NULL);

  return status;
}

// ==========================================================================================================
// Обработать первые в очереди ожидания Запросы для всех доступных систем сбора
int EGSA::process_requests_for_all_sites()
{
  int rc = NOK;
  SmedObjectList<AcqSiteEntry*> *all_sa = m_smed->SiteList();

  for(std::size_t i=0; i < all_sa->count(); i++) {

    AcqSiteEntry* site = all_sa->operator[](i);
    //assert(site);
    if (site) {
      //
      rc = process_request_for_site(site);
      LOG(INFO) << site->name() << ": requests processing status=" << rc;
    }
    else {
      LOG(WARNING) << "process_requests_for_all_sites: site #" << i << " is NULL";
    }

  }

  return rc;
}

// ==========================================================================================================
// Обработать первый Запрос из в очереди ожидания указанной системы сбора
// После обработки запроса (перехода в состояние EXECUTED) он удаляется.
// Если хотя бы один и группы composed-запросов завершился с ошибкой, вся группа считается завершившейся с
// ошибкой, и подзапросы группы удаляются.
int EGSA::process_request_for_site(AcqSiteEntry* site)
{
  int rc = OK;

  if (!site->requests().empty()) {

      // Всегда исполнять запросы по очереди.
      // Пока не выполнится текущий, не переходить к исполнению следующих
      Request *executed = site->requests().front();

      LOG(INFO) << site->name() << ": run " << executed->name() << " [" << executed->str_state() << "]";

      switch(executed->state()) {
        // -----------------------------------------------------------------
        case Request::STATE_INPROGRESS:  // На исполнении
          executed->state(Request::STATE_EXECUTED);
          break;

        // -----------------------------------------------------------------
        case Request::STATE_ACCEPTED:    // Принят к исполнению
          // в режим ожидания, если не подразумевается немедленного ответа
          executed->state(Request::STATE_WAIT_N);
          break;

        // -----------------------------------------------------------------
        case Request::STATE_SENT:        // Отправлен
          // после получения подтверждения о приеме
          executed->state(Request::STATE_ACCEPTED);
          break;

        // -----------------------------------------------------------------
        case Request::STATE_EXECUTED:    // Исполнен
          // Удалить запрос из списка
          LOG(INFO) << "Done request " << executed->name() << " last_in_bundle?:" << executed->last_in_bundle();
          delete executed;
          site->requests().pop_front();
          break;

        // -----------------------------------------------------------------
        case Request::STATE_ERROR:       // При выполнении возникла ошибка
          LOG(INFO) << "Error request " << executed->name();
          // TODO: удалить оставшиеся подзапросы из этой группы.
          // Для этого каждый подзапрос должен знать идентификатор своего composed-запроса
          rc = NOK;
          break;

        // -----------------------------------------------------------------
        case Request::STATE_NOTSENT:     // Еще не отправлен
          // TODO: отправить в адрес СС
          executed->state(Request::STATE_SENT);
          break;

        // -----------------------------------------------------------------
        case Request::STATE_WAIT_N:      // Ожидает (нормальный приоритет)
        case Request::STATE_WAIT_U:      // Ожидает (высокий приоритет)
          // ожидать ответа на запрос от СС, может быть:
          // ERROR = Ошибка
          // EXECUTED = Исполнено
          // INPROGRESS = Промежуточное состояние - ещё в процессе выполнения
          executed->state(Request::STATE_INPROGRESS);
          break;

        // -----------------------------------------------------------------
        case Request::STATE_SENT_N:      // Отправлен (нормальный приоритет)
        case Request::STATE_SENT_U:      // Отправлен (высокий приоритет)
          executed->state(Request::STATE_EXECUTED);
          break;

        // -----------------------------------------------------------------
        case Request::STATE_UNKNOWN:
          LOG(ERROR) << site->name() << ": unsupported request " << executed->name() << " state: " << executed->state();
          rc = NOK;
      }
  }

  return rc;
}

// ==========================================================================================================
bool EGSA::change_egsa_state_to(internal_state_t _state)
{
  bool changed = false;

  if (m_state_egsa != _state) {
    LOG(INFO) << "EGSA state changes " << m_state_egsa << " => " << _state;
    changed = true;
    m_state_egsa = _state;
  }

  return changed;
}

// ==========================================================================================================
bool EGSA::change_acq_state_to(internal_state_t _state)
{
  bool changed = false;

  if (m_state_acq != _state) {
    LOG(INFO) << "ES_ACQ state changes " << m_state_acq << " => " << _state;
    changed = true;
    m_state_acq = _state;
  }

  return changed;
}

// ==========================================================================================================
bool EGSA::change_send_state_to(internal_state_t _state)
{
  bool changed = false;

  if (m_state_send != _state) {
    LOG(INFO) << "ES_SEND state changes " << m_state_send << " => " << _state;
    changed = true;
    m_state_send = _state;
  }

  return changed;
}

// ==========================================================================================================
// Ответ на простой запрос верхнего уровня.
// Отправить строку и код состояния.
int EGSA::esg_ine_ini_Acknowledge(const std::string& origin, int message_type, int code)
{
  static const char* fname = "esg_ine_ini_Acknowledge";
  int rc = NOK;

  LOG(INFO) << fname << ": send msg id=" << message_type << " to " << origin << ", code=" << code;
  return rc;
}

// ==========================================================================================================
// 1. Дождаться сообщения ADDL о запросе готовности к работе
// 2. Прочитать конфигурационный файл
// 3. Инициализация подписки на атрибуты состояний Систем Сбора
// 4. Ответить на запрос готовности к работе
//    Если инициализация прошла неуспешно - выйти из программы
// 5. Дождаться сообщения ADDL о начале штатной работы
// 6. Цикл до получения сообщения об останове:
// 6.1. Проверять наступление очередного цикла опроса
// 6.2. Опрашивать пробуждения (?)
// 6.3. Опрашивать состояния СС
//
int EGSA::run()
{
  const char* fname = "EGSA::run";
  int status = OK;
  bool recv_timeout = false;
  std::string *reply_to = new std::string;
  // Устанавливается флаг в случае получения команды на "Останов" из-вне
  bool get_order_to_stop = false;
#ifdef _FUNCTIONAL_TEST
  int sec=0;
#endif

  interrupt_worker = true;
  // Получить информацию из БДРВ о названии (коде) локального Сайта
  status = get_local_info();

  LOG(INFO) << fname << ": START";

  switch(m_state) {
    case STATE_INI_OK:
      // Инициализация уже была выполнена
      LOG(WARNING) << fname << ": config already loaded";
      break;

    case STATE_INITIAL:
      LOG(WARNING) << fname << ": need to load config";

      // Загрузка конфигурации EGSA
      if ((OK == status) && (true == init())) {
        // Загрузка основной части кофигурации SA и создание экземпляров SMAD без фактического подключения
        if (OK == (status = attach_to_sites_smad())) {
          interrupt_worker = false;
        }
      }
      break;

    default:
      LOG(ERROR) << fname << ": current state is " << m_state;
      status = NOK;
  }

  if (STATE_INI_OK != m_state) {
    LOG(FATAL) << fname << ": unable to continue, state=" << m_state;
    delete reply_to;
    return NOK;
  }

  try
  {
    // Запустить нить интерфейса
    m_servant_thread = new std::thread(std::bind(&EGSA::implementation, this));
    // Нить es_acq
    m_acquisition_thread = new std::thread(std::bind(&EGSA::implementation_acq, this));
    // Нить es_send
    m_sending_thread = new std::thread(std::bind(&EGSA::implementation_send, this));

    m_state = STATE_RUN;

    // Ожидание завершения работы Прокси
    while (!interrupt_worker && (STATE_SHUTTINGDOWN != m_state))
    {
      mdp::zmsg   *request  = NULL;

      LOG(INFO) << fname << " ready";
#ifndef _FUNCTIONAL_TEST
      request = mdwrk::recv (reply_to, 1000, &recv_timeout);
#else
      if (sec < 20) { // тест от 0 до 20 секунды
        sleep(1);
        recv_timeout = 1; // Признак завершения чтения по таймауту
        LOG(INFO) << fname << " Test iteration, " << sec << " second(s)";
        sec++;
      }
      else {    // завершаем тест
        LOG(INFO) << fname << " Test finish, " << sec << " second(s)";
        recv_timeout = 0;
        request = NULL;
      }
#endif
      if (request)
      {
        LOG(INFO) << fname << " Got a message";

        // Обслуживание внешних запросов от Брокера и Клиентов
        processing(request, *reply_to, get_order_to_stop);

        // Обработка очередного цикла из очереди
        cycles::timer();

        if (get_order_to_stop) {
          LOG(INFO) << fname << " Got a shutdown order";
          interrupt_worker = true; // order to shutting down
          m_state = STATE_SHUTTINGDOWN;
        }

        delete request;
      }
      else
      {
        if (!recv_timeout) {
          LOG(INFO) << fname << " Got a NULL";
          interrupt_worker = true; // Worker has been interrupted
          m_state = STATE_SHUTTINGDOWN;
        }
        else {
          // Все в порядке, за период опроса не было сообщений
        }
      }
    }

    LOG(INFO) << fname << ": joining acquisition thread -> start";
    m_acquisition_thread->join();
    delete m_acquisition_thread;
    LOG(INFO) << fname << ": joining acquisition thread -> done";

    LOG(INFO) << fname << ": joining sending thread -> start";
    m_sending_thread->join();
    delete m_sending_thread;
    LOG(INFO) << fname << ": joining sending thread -> done";

    LOG(INFO) << fname << ": joining implementation thread -> start";
    m_servant_thread->join();
    delete m_servant_thread;
    LOG(INFO) << fname << ": joining implementation thread -> done";

    delete reply_to;

    LOG(INFO) << fname << ": message processing cycle is finished";

  }
  catch(zmq::error_t error)
  {
    LOG(ERROR) << fname << ": transport error catch: " << error.what();
    interrupt_worker = true;
    status = NOK;
  }
  catch (std::exception &e)
  {
    LOG(ERROR) << fname << ": runtime signal catch: " << e.what();
    interrupt_worker = true;
    status = NOK;
  }

  // Ресурсы очищаются в локальной нити, деструктору ничего не достанется...
  LOG(INFO) << fname << ": STOP";

  m_state = STATE_STOP;

  return status;
}

// ==========================================================================================================
// Получить информацию по локальному Сайту из БДРВ
int EGSA::get_local_info()
{
  int rc = OK;

#ifndef _FUNCTIONAL_TEST
  #warning "TODO: Get name of local SA"
  // NB: У Сайта есть атрибут уровня, возвращается функцией level(). Значение LEVEL_LOCAL говорит о том, что именно этот Сайт - локальный
  rc = NOK;
  LOG(ERROR) << "TODO: Get name of local SA";
  m_local_sa_name[0] = '\0';
#else
  strcpy(m_local_sa_name, "K42663");
  LOG(INFO) << "USE FUNCTIONAL_TEST: local SA name is " << m_local_sa_name;
#endif

  return rc;
}

// ==========================================================================================================
void EGSA::tick_tack()
{
  cycles::timer();
}

#if 0
// ==========================================================================================================
// Отправить всем подчиненным системам запрос готовности, и ожидать ответа
void EGSA::fire_ENDALLINIT()
{
#if (VERBOSE > 5)
  const char *fname = "fire_ENDALLINIT";
#endif
  bool stop_status;
  msg::SimpleRequest *simple_request = NULL;
//  msg::SimpleReply   *simple_reply = NULL;
  mdp::zmsg          *send_msg  = NULL;
  mdp::zmsg          *recv_msg  = NULL;
  std::string        *reply_to = new std::string;
  AcqSiteEntry       *sa;
//1  auto now = std::chrono::system_clock::now();

  LOG(INFO) << "Start sending ENDALLINIT to all SA";

  simple_request = static_cast<msg::SimpleRequest*>(m_message_factory->create(ADG_D_MSG_ENDALLINIT));

  for (size_t idx = 0; idx < m_ega_ega_odm_ar_AcqSites.count(); idx++)
  {
    sa = m_ega_ega_odm_ar_AcqSites[idx];
    assert(sa);

    switch(sa->state())
    {
      case SA_STATE_UNREACH: // Недоступна
        LOG(INFO) << sa->name() << " state is UNREACHABLE";
        break;

      case SA_STATE_OPER:    // Оперативная работа
        LOG(WARNING) << sa->name() << " state is OPERATIONAL before ENDALLINIT";
        break;

      case SA_STATE_PRE_OPER:// В процессе инициализации
        LOG(WARNING) << sa->name() << " state is PRE OPERATIONAL";
        break;

      case SA_STATE_INHIBITED:      // СС в состоянии запрета работы
      case SA_STATE_FAULT:          // СС в сбойном состоянии
      case SA_STATE_DISCONNECTED:   // СС не подключена
        break;


      case SA_STATE_UNKNOWN: // Неопределенное состояние
      default:
        LOG(ERROR) << sa->name() << " is in unsupported state:" << sa->state();
        break;
    }

    LOG(INFO) << "Send ENDALLINIT to " << sa->name();

    simple_request->header()->set_proc_dest(sa->name());
    send_msg = simple_request->get_zmsg();
    send_msg->wrap(sa->name(), "");

    send_to_broker((char*) MDPW_REPORT, NULL, send_msg);

/*
    if (sa->send(ADG_D_MSG_ENDALLINIT)) {
      cycles::add(std::bind(&SystemAcquisition::check_ENDALLINIT, sa), now + std::chrono::seconds(2));
    }
*/
  }

  // TODO: Проверить, кто из СС успел передать сообщение о завершении инициализации
  int steps = 6;
  while(--steps) {

    // NB: Функция recv возвращает только PERSISTENT-сообщения
    recv_msg = mdwrk::recv (reply_to, 1000);
    if (recv_msg)
    {
#if (VERBOSE > 5)
      LOG(INFO) << fname << ": got a message from " << reply_to;
#endif

      processing (recv_msg, *reply_to, stop_status);

      delete recv_msg;
    }

    cycles::timer();
  }
}
#endif

// ==========================================================================================================
// Активировать циклы
int EGSA::activate_cycles()
{
  auto now = std::chrono::system_clock::now();
  //int site_idx; // Порядковый номер Сайта в списке для данного Цикла

  // Пройти по всем возможным Циклам
  for (size_t idx = 0; idx < cycles()->count(); idx++)
  {
    Cycle* cycle = (*cycles())[idx];

    // Для каждого Цикла активировать столько экземпляров, сколько в нем объявлено Сайтов
    for (size_t site_idx = 0; site_idx < cycle->sites().size(); site_idx++)
    {
      LOG(INFO) << "Activate cycle: " << cycle->name()
                << " id=" << cycle->id()
                << " period=" << cycle->period()
                << " SA: " << cycle->sites().at(site_idx)->name() << " id=" << site_idx;

      // Связать триггер с объектом, идентификаторами Цикла и Системы Сбора
      cycles::add(std::bind(&EGSA::cycle_trigger, this, cycle->id(), site_idx),
                  now /*+ std::chrono::seconds((*it)->period())*/); // Время активации: сейчас (+ период цикла?)
    }
  }

  return OK;
}

// ==========================================================================================================
// Деактивировать циклы
int EGSA::deactivate_cycles()
{
//  int cycle_idx = 0;

  for (size_t idx = 0; idx < cycles()->count(); idx++) {
    LOG(INFO) << "Deactivate cycle id=" << idx << ": " << (*cycles())[idx]->name();
  }
  cycles::clear();

  return OK;
}

// ==========================================================================================================
// Функция срабатывания при наступлении времени очередного таймера
// На входе
// cycle_id : идентификатор номера Цикла
// sa_id    : локальный идентификатор СС для этого Цикла (NB: они не совпадают с id из m_ega_ega_odm_ar_Sites)
// Для каждой СС периоды Циклов одинаковы, но время начала индивидуально.
// sa_id == -1 означает, что Цикл общий для всех его СС.
void EGSA::cycle_trigger(size_t cycle_id, size_t sa_id)
{
  int rc = OK;
#if VERBOSE>6
  int delta_sec;
#endif

  assert(cycle_id < cycles()->count());
  Cycle *cycle = (*cycles())[cycle_id];
  assert(cycle);
  assert(sa_id < cycle->sites().size());
  AcqSiteEntry *site = cycle->sites().at(sa_id);
  assert(site);

  LOG(INFO) << "Trigger callback for cycle id=" << cycle_id << " (" << cycle->name() << ") "
            << " SA id=" << sa_id << " (" << site->name() << ")";

  if (cycle_id < ID_CYCLE_UNKNOWN) {
#if VERBOSE>6
    delta_sec = cycle->period();
#endif

    if (OK == rc) {
#if VERBOSE>6
      LOG(INFO) << "Reactivate cycle " << cycle->name() << " (" << cycle_id << "," << sa_id << ")";
#endif
      auto now = std::chrono::system_clock::now();
      cycles::add(std::bind(&EGSA::cycle_trigger, this, cycle_id, sa_id),
                  now + std::chrono::seconds(cycle->period()));

      // TODO: Проверить текущее состояние Сайта. В зависимости от него добавить новые или удалить
      // уже существующие, еще незавершенные, Запросы
      switch(site->state())
      {
        case EGA_EGA_AUT_D_STATE_NI_NM_NO:  // Acquisition systems requests only
          site->release_requests(INFO|SERV);
          break;
        case EGA_EGA_AUT_D_STATE_NI_M_NO:   // Only commands processing in maintaince mode
          site->release_requests(ACQSYS);
          break;
        case EGA_EGA_AUT_D_STATE_NI_NM_O:   // Normal operation state
          break;
        case EGA_EGA_AUT_D_STATE_NI_M_O:    // Dispatcher request only
          site->release_requests(ACQSYS);
          break;
        case EGA_EGA_AUT_D_STATE_I_NM_NO:   // Inhibition
        case EGA_EGA_AUT_D_STATE_I_M_NO:    // Inhibition
        case EGA_EGA_AUT_D_STATE_I_NM_O:    // Inhibition
        case EGA_EGA_AUT_D_STATE_I_M_O:     // Inhibition
          site->release_requests(ALL);
          break;
      }





#if VERBOSE>6
      auto next = now + std::chrono::seconds(delta_sec);
      time_t tt_now    = std::chrono::system_clock::to_time_t ( now );
      time_t tt_future = std::chrono::system_clock::to_time_t ( next );
      LOG(INFO) << "now is: " << ctime(&tt_now);
      LOG(INFO) << "future is: " << ctime(&tt_future);
#endif
    }
  }
  else {
      LOG(ERROR) << "Unsupported cycle_id=" << cycle_id << " for SA id=" << sa_id;
      rc = NOK;
  }
}

// ==========================================================================================================
// Тестовая функция ожидания срабатывания таймеров в течении заданного времени
int EGSA::wait(int max_wait_sec)
{
  //fd_set rfds;
  //struct timeval tv;
  //int retval;
  //char buffer[100 + 1];
  //ssize_t read_sz;
  //ssize_t buffer_sz = 100;

#if 0
  /* Watch fd[1] to see when it has input. */
  FD_ZERO(&rfds);
  FD_SET(m_socket, &rfds);
#endif

  for (int i=0; i<max_wait_sec; i++) {
    if (interrupt_worker) {
      LOG(INFO) << "interrupt EGSA::wait";
      break;
    }

#if 1
    cycles::timer();
    std::cout << "." << std::flush;
    sleep(1);
#else
    /* Wait up to one seconds. */
    tv.tv_sec = 1;
    tv.tv_usec = 1;

    retval = select(m_socket+1, &rfds, NULL, NULL, &tv);
    /* Don't rely on the value of tv now! */

    switch(retval) {
      case -1:
        perror("select()");
        break;

      case 0:
        cycles::timer();
        std::cout << "." << std::flush;
        break;

      default:
        cycles::timer();
        read_sz = read(m_socket, buffer, buffer_sz);
        buffer[(read_sz >= 0)? read_sz : 0] = '\0';
        LOG(INFO) << "Read " << read_sz << " bytes: " << buffer;
        /* FD_ISSET(0, &rfds) will be true. */
    }
#endif
  }
  std::cout << std::endl;

  return OK;
}

// ==========================================================================================================
// Функция первично обрабатывает полученный запрос.
// Обслуживаемые запросы перечислены в egsa.mkd
//
// NB: Запросы обрабатываются последовательно.
int EGSA::processing(mdp::zmsg* request, const std::string &identity, bool& need_to_stop)
{
  rtdbMsgType msgType;
  int rc = OK;

//  LOG(INFO) << "Process new request with " << request->parts()
//            << " parts and reply to " << identity;

  // Получить отметку времени начала обработки запроса
  // m_metric_center->before();

  msg::Letter *letter = m_message_factory->create(request);
  if (letter && letter->valid())
  {
    msgType = letter->header()->usr_msg_type();

    switch(msgType)
    {
      case ADG_D_MSG_ASKLIFE:
        rc = handle_asklife(letter, identity);
        break;

      // Завершение инициализации
      case ADG_D_MSG_ENDALLINIT:
        rc = handle_end_all_init(letter, identity);
        break;

      // Чтение данных
      case SIG_D_MSG_READ_MULTI:
        rc = handle_read_multiple(letter, identity);
        break;

      // Обновление группы подписки
      case SIG_D_MSG_GRPSBS:
        rc = handle_sbs_update(letter, identity);
        break;

      // Запрос на телерегулирование
      case SIG_D_MSG_ECHCTLPRESS:
      case SIG_D_MSG_ECHDIRCTLPRESS:
        rc = handle_teleregulation(letter, identity);
        break;

      case ADG_D_MSG_STOP:
        rc = handle_stop(letter, identity);
        need_to_stop = true;
        break;

      default:
        LOG(ERROR) << "Unsupported request type: " << msgType;
    }
  }
  else
  {
    LOG(ERROR) << "Received letter "<<letter->header()->exchange_id()<<" not valid";
  }

  if (NOK == rc) {
    LOG(ERROR) << "Failure processing request: " << msgType;
  }

  delete letter;

  // Получить отметку времени завершения обработки запроса
  // m_metric_center->after();

  return rc;
}

// ==========================================================================================================
// Останов экземпляра
int EGSA::stop()
{
  static const char* fname = "stop";
  int rc = OK;

  LOG(INFO) << fname << ": get request to stop component";
  interrupt_worker = true;

  return rc;
}

// ==========================================================================================================
// Обработка сообщения внутри нити ES_ACQ
int EGSA::process_acq(const std::string& command)
{
  static const char* fname = "process_acq";
  int rc = OK;

  LOG(INFO) << fname << ": get request " << command << " from ES_ACQ";

  return rc;
}

// ==========================================================================================================
// Обработка сообщения внутри нити ES_SEND
int EGSA::process_send(const std::string& command)
{
  static const char* fname = "process_send";
  int rc = OK;

  LOG(INFO) << fname << ": get request " << command << " from ES_SEND";

  return rc;
}

// ==========================================================================================================
// Источник: сторожевая схема
// Условия получения: завершение инициализации Обменов
// Результат: старт нормальной работы Обменов
//
// Порядок выполнения процедуры:
// 1) Для каждой системы сбора - активация циклов
// 2) Отправка ответа EXEC_RESULT по завершению этапа
int EGSA::handle_end_all_init(msg::Letter*, const std::string& origin)
{
  const char* fname = "EGSA::handle_end_all_init";

  LOG(INFO) << fname << ": BEGIN (from " << origin << ")";
  // TODO: Передать команду Начало Работы в implementation, Циклы должны активироваться там
  // activate_cycles();
  LOG(INFO) << fname << ": END";

  return OK;
}

// ==========================================================================================================
int EGSA::handle_teleregulation(msg::Letter*, const std::string& origin)
{
  LOG(ERROR) << "Not yet realized: handle_teleregulation from " << origin;
  return NOK;
}

// ==========================================================================================================
int EGSA::handle_read_multiple(msg::Letter*, const std::string& origin)
{
  LOG(ERROR) << "Not yet realized: handle_read_multiple from " << origin;
  return NOK;
}

// ==========================================================================================================
int EGSA::handle_sbs_update(msg::Letter*, const std::string& origin)
{
  int rc = NOK;

  LOG(ERROR) << "Not yet realized: handle_sbs_update from " << origin;

  // TODO: при изменении какого-либо атрибута СС (SYNTHSTATE, INHIBITION, EXPMODE) пересчитать состояние СС
  // RTDB_ATT_IDX_SYNTHSTATE
  // RTDB_ATT_IDX_INHIB
  // RTDB_ATT_IDX_EXPMODE
  //
#warning "Recalculate SA state on SBS of SA attributes receiving"
  return rc;
}

// ==========================================================================================================
// Активация группы подписки точек систем сбора
int EGSA::activateSBS()
{
  std::string sbs_name = EXCHANGE_NAME;
  std::string rtdb_service_name = RTDB_NAME;
  int status = NOK;
  msg::Letter *request = NULL;
  mdp::zmsg* transport_cell;
  // Управление Группами Подписки
  msg::SubscriptionControl *sbs_ctrl_msg = NULL;

  try {
      // Инициируем создание сокета получения сообщений от сервера подписки
      // NB: (2016-05-24) mdp::mdwrk может только регистрировать одну подписку от БДРВ
      if (subscribe_to(RTDB_NAME, sbs_name)) {
        // Сообщим Службе о готовности к приему данных
        request = m_message_factory->create(SIG_D_MSG_GRPSBS_CTRL);
        sbs_ctrl_msg = dynamic_cast<msg::SubscriptionControl*>(request);
        sbs_ctrl_msg->set_name(sbs_name);
        // TODO: ввести перечисление возможных кодов
        // Сейчас (2015-10-01) 0 = SUSPEND, 1 = ENABLE
        sbs_ctrl_msg->set_ctrl(1);

        request->set_destination(rtdb_service_name);
        transport_cell = request->get_zmsg();

        if (send_to(RTDB_NAME, transport_cell, DIRECT)) {
          // От сервера подписки получить и разобрать ответ, содержащий
          // начальные значения атрибутов из группы подписки "EGSA"
          status = waitSBS();
        }
        else {
          LOG(ERROR) << "Subscribing failure";
        }

        delete transport_cell;
        delete request;
      }
      else {
        LOG(ERROR) << "Can't activate SA states group";
      }
  }
  catch(zmq::error_t err)
  {
    LOG(ERROR) << err.what();
  }
  catch (std::exception &e)
  {
    LOG(ERROR) << "catch the signal: " << e.what();
  }

  return status;
}

// ==========================================================================================================
int EGSA::recv(msg::Letter* &result, int timeout_msec)
{
  int status = OK;
  mdp::zmsg* report = NULL;
  std::string *reply_to = NULL;
  bool timeout_sign = false;

  assert(&result);
  result = NULL;
  reply_to = new std::string;

  if (NULL == (report = mdwrk::recv(reply_to, timeout_msec, &timeout_sign)))
  {
    if (!timeout_sign) {
      LOG(ERROR) << "Receiving failure";
      // Возможно, Брокер не запущен
      status = NOK;
    }
    else {
      // Всё в порядке, но за указанное время новых сообщений не поступало
      LOG(INFO) << "Receiving timeout";
    }
  }
  else
  {
    result = m_message_factory->create(report);
    delete report;
  }

  delete reply_to;

  return status;
}

// ==========================================================================================================
// Ожидать получение сообщения SIG_D_MSG_READ_MULTI с первоначальными данными по группе подписки EGSA
int EGSA::waitSBS()
{
  int status = NOK;
  std::string cause_text = "";
  //msg::ExecResult *resp = NULL;
  msg::Letter *report = NULL;
  bool stop_receiving = false;

  try
  {
    //  Wait for report
    while (!stop_receiving)
    {
      LOG(INFO) << "Wait receiving SBS answer";
      // Ждать 2 секунды ответа с начальными значениями группы подписки
      status = recv(report, 2000);
      // Что-то получили?
      if (report) {
        // И полученное сообщение прочиталось?
        if (report->valid()) {

          if (SIG_D_MSG_READ_MULTI == report->header()->usr_msg_type()) {
            // Для режима подписки это сообщение приходят однократно,
            // для инициализации начальных значений атрибутов из группы
            LOG(INFO) << "Got SIG_D_MSG_READ_MULTI response: " << report->header()->usr_msg_type();

            status = process_read_response(report);
          }
          else {
            LOG(ERROR) << "Get unwilling message type #" << report->header()->usr_msg_type()
                       << " while waiting #" << SIG_D_MSG_READ_MULTI;
#if (VERBOSE > 5)
            report->dump();
#endif
          }
          stop_receiving = true;
          delete report;
        }
        else {
         stop_receiving = true;
         LOG(INFO) << "Receive broken SBS message";
        }
      }
      else {
        LOG(ERROR) << "None SBS received, status=" << status << ", exiting";
        stop_receiving = true;
      }
    }
  }
  catch (zmq::error_t err)
  {
    LOG(ERROR) << err.what();
  }

  return status;
}

// ==========================================================================================================
// Обработка полученных по подписке от БДРВ данных
int EGSA::process_read_response(msg::Letter* report)
{
  int attribute_idx = RTDB_ATT_IDX_UNEXIST;
  int status = OK;
  msg::ReadMulti* response = dynamic_cast<msg::ReadMulti*>(report);

  for (std::size_t idx = 0; idx < response->num_items(); idx++)
  {
    const msg::Value& attr_val = response->item(idx);
    LOG(INFO) << attr_val.tag() << " = " << attr_val.as_string() << std::endl;

    if (std::string::npos != attr_val.tag().find(RTDB_ATT_SYNTHSTATE)) {
      attribute_idx = RTDB_ATT_IDX_SYNTHSTATE;
    }
    else if (std::string::npos != attr_val.tag().find(RTDB_ATT_INHIB)) {
      attribute_idx = RTDB_ATT_IDX_INHIB;
    }
    else if (std::string::npos != attr_val.tag().find(RTDB_ATT_EXPMODE)) {
      attribute_idx = RTDB_ATT_IDX_EXPMODE;
    }
    else {
      LOG(WARNING) << "Subscription processing: skip unsupported atribute " << attr_val.tag();
    }

    // Проверим, получили ли мы допустимый атрибут
    if (RTDB_ATT_IDX_UNEXIST != attribute_idx) { // Да, один из допустимых
      // Обновим состояние Сайтов в SMED
      //
      // Вырезать из attr_val.tag() часть от первого символа '/' до '.'
      const size_t point_pos = attr_val.tag().find(".");
      assert(std::string::npos != point_pos);
      assert(point_pos > 1);

      const std::string sa_name = attr_val.tag().substr(1, point_pos-1);
      AcqSiteEntry* sa_entry = (*m_smed->SiteList())[sa_name];

      // Нашли в своей конфигурации упоминание этой СС
      if (sa_entry) {
        // Преобразовали числовой код состояния SYNTHSTATE из БДРВ
        // const sa_state_t new_state = int_to_sa_state(attr_val.raw().fixed.val_uint8);
        // Зарядили необходимые Запросы в Циклы, где участвует эта СС

        // Поменяли состояние СС на нужное
        sa_entry->esg_esg_aut_StateManage(attribute_idx, attr_val.raw().fixed.val_uint8);
        LOG(INFO) << "Update SA " << sa_name << " state to " << sa_entry->state();
      }
      else {
        LOG(WARNING) << "Skip processing SA " << sa_name << " because of absense it's configuration";
      }


    }

  }

  return status;
}

// ==========================================================================================================
int EGSA::handle_asklife(msg::Letter* letter, const std::string& identity)
{
  msg::AskLife   *msg_ask_life = static_cast<msg::AskLife*>(letter);
  mdp::zmsg      *response = new mdp::zmsg();
  int exec_val = 1;

  msg_ask_life->set_exec_result(exec_val);

  response->push_front(const_cast<std::string&>(msg_ask_life->data()->get_serialized()));
  response->push_front(const_cast<std::string&>(msg_ask_life->header()->get_serialized()));
  response->wrap(identity.c_str(), EMPTY_FRAME);

  LOG(INFO) << "Processing asklife from " << identity
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
int EGSA::handle_stop(msg::Letter* letter, const std::string& identity)
{
  int rc = OK;
  const char* fname = "handle_stop";
  msg::ExecResult *exec_reply = static_cast<msg::ExecResult*>(m_message_factory->create(ADG_D_MSG_EXECRESULT));

  LOG(INFO) << fname << ": BEGIN ADG_D_MSG_STOP from " << identity;
  interrupt_worker = true;

  try
  {
    // Отправить сообщение в EGSA о принудительном останове
    LOG(INFO) << "Send TERMINATE to EGSA implementation threads";
    m_backend_socket.send("TERMINATE", 9, 0);

    deactivate_cycles();

    // TODO: Проверить, что все ресурсы освобождены, а нити остановлены
    exec_reply->header()->set_exchange_id(letter->header()->exchange_id());
    // TODO: Возможно ли переполнение?
    exec_reply->header()->set_interest_id(letter->header()->interest_id() + 1);
    exec_reply->header()->set_proc_dest(identity);
    if (OK == rc)
      exec_reply->set_exec_result(1);
    else
      exec_reply->set_failure_cause(0, "GEV");

    mdp::zmsg *response = exec_reply->get_zmsg();
    response->wrap(identity.c_str(), "");

    // Послать отправителю сообщение EXEC_RESULT
    LOG(INFO) << "Send EXEC_RESULT to " << identity;
    send_to_broker((char*) MDPW_REPORT, NULL, response);

    delete response;
  }
  catch(zmq::error_t error)
  {
    LOG(ERROR) << fname << ": catch: " << error.what();
    rc = NOK;
  }
  catch(std::exception &e)
  {
    LOG(ERROR) << e.what();
    rc = NOK;
  }

  delete exec_reply;

  LOG(INFO) << fname << ": END ADG_D_MSG_STOP";

  return rc;
}

#if 0
// ==========================================================================================================
// Получить набор циклов, в которых участвует заданная СС
std::vector<Cycle*> *EGSA::get_Cycles_for_SA(const std::string& sa_name)
{
  std::vector<Cycle*> *search_result = NULL;

  // СС может быть заявлена в нескольких циклах, проверить их все
  for (size_t idx = 0; idx < m_ega_ega_odm_ar_Cycles.count(); idx++)
  {
    //LOG(INFO) << sa_name << ":" << (*it)->name();
    Cycle* cycle = m_ega_ega_odm_ar_Cycles[idx];
    // Если указанная СС заявлена в данном цикле
    if (cycle && cycle->exist_for_SA(sa_name)) {

      //LOG(INFO) << sa_name << ": found cycle " << (*it)->name();

      // Создать вектор с циклами, если это не было сделано ранее
      if (!search_result) search_result = new std::vector<Cycle*>;
      search_result->push_back(cycle);
    }

  }

  return search_result;
}

// ==========================================================================================================
// Получить набор запросов, зарегистрированных за данной СС
std::vector<Request*> *EGSA::get_Request_for_SA(const std::string& sa_name)
{
  return NULL;
}

#endif

// ==============================================================================
// Обработка события смены Фазы для данного Сайта
int EGSA::esg_acq_inm_PhaseChgMan(AcqSiteEntry *site, AcqSiteEntry::init_phase_state_t i_InitPhase)
{
  static const char* fname = "EGSA::esg_acq_inm_PhaseChgMan";
  int rc = NOK;
  int i_LastDistantInitPhase = site->InitPhase();
  int i_StateValue;

  site->InitPhase(i_InitPhase);

  LOG(INFO) << site->name()
            << " LastDistantInitPhase=" << i_LastDistantInitPhase
            << " OPStateAuthorised=" << site->OPStateAuthorised()
            << " DistantInitTerminated=" << site->DistantInitTerminated()
            << " DistantInitPhase=" << site->InitPhase()
            << " FirstDistInitOPStateAuth=" << site->FirstDistInitOPStateAuth();

  // if init - local and distant site are finished
  // then set synth. state to operational
  //
  // if local init is terminated and distant site restarts an init
  // then local site has also to restart init
  //
  // add new End init phase with no HHist

  if (true == site->OPStateAuthorised()) {
    switch(i_InitPhase) {
      case AcqSiteEntry::INITPHASE_END:
      case AcqSiteEntry::INITPHASE_END_NOHHIST:
        i_StateValue = SYNTHSTATE_OPER;
        // Transmit immediatly the new synthetic state to SINF component
        rc = esg_acq_dac_SynthStateMan (site, i_StateValue);
        if (rc == OK) {
          site->OPStateAuthorised(false);
          site->DistantInitTerminated(false);
          site->FirstDistInitOPStateAuth(false);
        }
        break;

      default:
        if (true == site->FirstDistInitOPStateAuth()) {
          site->FirstDistInitOPStateAuth(false);
        }
        else {
          if (i_LastDistantInitPhase == AcqSiteEntry::INITPHASE_BEGIN) {
            site->OPStateAuthorised(false);
          }
        }
    } // end switch
  }  // end if OPStateAuthorised
  else {
  // Init - site local en cours ou pas realisee
  // --------------------------------------------------------------------------
// Init - site distant terminee alors le memoriser
// --------------------------------------------------------------------------
// add new End init phase with no HHist
            if ((i_InitPhase == AcqSiteEntry::INITPHASE_END) || (i_InitPhase == AcqSiteEntry::INITPHASE_END_NOHHIST)) {
                site->DistantInitTerminated(true);
            }
            else {
                site->DistantInitTerminated(false);
            }

            LOG(INFO) << site->name() << ": Update DistantInitTerminated " << site->DistantInitTerminated();
// sur init site distant passe etat synthetique
//                    a pre-operationnel s'il etait operationnel
//                    afin de prendre en compte la coupure de liaison
//                    detecter par le site distant;
//                    le site operationnel detecte cette coupure par la 1iere
//                    requete d'init recue : en INITPHASE_BEGIN pour une DIPL
//                                           en INITPHASE_END pour une DIR
// --------------------------------------------------------------------------
           if (rc == OK) {
               if (site->synthstate() == SYNTHSTATE_OPER) {
                   i_StateValue = SYNTHSTATE_PRE_OPER;
                   // Transmit immediatly the new synthetic state to SINF component
                   rc = esg_acq_dac_SynthStateMan (site, i_StateValue);
                   if (rc == NOK) {
                     LOG(ERROR) << fname << ": Update smed et send to SINF synth state";
                   }
                   else {
                       site->OPStateAuthorised(false);
                       // delete request in progress
                       rc = site->esg_ine_man_DeleteReqInProgress ("REQ" /*ESG_ESG_FIL_D_SUFF_REQUEST*/);
                       if (rc == NOK) {
                         LOG(ERROR) << fname << ": site=" << site->name() << ", new StateValue=" << i_StateValue;
                       } // End if: Error
                   }
               }
           }
        } // end else: if site is not OPStateAuthorised

  LOG(INFO) << fname << ": End stat=" << rc;

  return rc;
}

// ==========================================================================================================
// Удалить просроченные Запросы
int EGSA::esg_acq_rsh_ReqsDelMan()
{
  static const char* fname = "esg_acq_rsh_ReqsDelMan";
  int rc = NOK;

  LOG(INFO)  << fname << ": run";
  return rc;
}
// ==========================================================================================================

// Разбор файла с данными от ГОФО
int EGSA::load_esg_file(const char* filename)
{
  static const char* fname = "load_esg_file";
  int rc = NOK;

  LOG(INFO) << fname << ": run " << filename;

#if 0
  std::ifstream file(filename);
  if ( file )
  {
    std::stringstream buffer;

    buffer << file.rdbuf();

    file.close();

    translator()->load(buffer);
  }
#else
  rc = translator()->esg_acq_dac_Switch(filename);
#endif

  return rc;
}

// ==============================================================================
// Найти Запрос в адрес или от указанного Сайта по заданному идентификатору
// Все исполняемые запросы хранятся в БД (SMED?) в отдельной таблице. Базовая конфигурация запросов берется из НСИ egsa.json.
// 
// Запись таблицы исполняемых запросов содержит:
// 1) идентификатор запроса
// 2) ссылку на запись сайта в таблице НСИ Сайтов 
// 3) ссылку на соответствующую запись в таблице НСИ Запросов для этого Запроса
// 4) время выдачи запроса
// 5) максимальное время задержки ответа
// 6) состояние запроса request_executing_state_t (INPROGRESS, ACCEPTED, SENT, EXECUTED, ERROR, NOTSENT, WAIT, ...)
Request* EGSA::esg_esg_lis_ConsultBasic(AcqSiteEntry* site, int request_id, int request_type)
{
  Request *req = NULL;

  LOG(ERROR) << "Search req id #" << request_id << " for site " << site->name();
  return req;
}

// ==============================================================================
// Обработка изменения оперативного состояния системы сбора - атрибута SYNTHSTATE
int EGSA::esg_acq_dac_SynthStateMan(AcqSiteEntry* site, int _state)
{
  static const char* fname = "esg_acq_dac_SynthStateMan";
  int rc = OK;

  if (OK == (rc = site->SynthStateMan(_state)))
  {
    LOG(INFO) << "TODO: Propagate new SYNTHSTATE value to RTDB and SMED";

    // Обновить состояние Сайта в SMED
    // ----------------------------------------
    if (OK == (rc = smed()->esg_acq_dac_SmdSynthState (site->name(), site->synthstate()))) {

#if 1
      // Отправить изменившийся атрибут .SYNTHSTATE в БДРВ
      LOG(ERROR) << fname << ": send " << site->name() << "." << RTDB_ATT_SYNTHSTATE << " to RTDB";
#else

      // Build state message to be transmitted to DIFF process
      // -----------------------------------------------------
      memcpy(r_SyntSend.s_AcqSiteName, s_IAcqSiteId, sizeof(gof_t_UniversalName));
      r_SyntSend.i_StateValue = m_synthstate;

      // Consult general data to get DIFF process num
      // -----------------------------------------------------
      //   Build the structure process
      //   - Get the current Rtap environnement
      //   - Get the identification of the interface component
      // -----------------------------------------------------
      esg_esg_odm_ConsultGeneralData (&r_GenData);

      rc = gof_msg_p_GetProcByNum (r_GenData.s_RtapEnvironment, (gof_t_Int16)r_GenData.o_DIFprocNum, &r_ProcessDest);

      if (rc == OK)
      {
        rc = gof_msg_p_SendMessage (&r_ProcessDest, ESG_D_SYNTHETICAL_STATE, sizeof(esg_t_SyntheticalState), &r_SyntSend);

        // Send the message to es_Sending
        // -----------------------------------
        if (rc == OK) {
          rc = gof_msg_p_SendMessage (&r_ProcessDest, ESG_D_SYNTHETICAL_STATE, sizeof(esg_t_SyntheticalState), &r_SyntSend);
          // Transmit immediatly the new synthetic state to SINF component
          if (rc == OK) {
            strcpy (r_SynthStateSite.s_SA_name, s_IAcqSiteId);
            r_SynthStateSite.e_status = m_synthstate;

            rc = sig_ext_msg_p_InpSendMessageToSinf ( SIG_D_MSG_SASTATUS, sizeof (sig_t_msg_SAStatus), &r_SynthStateSite);
            sprintf ( s_Trace, "Transmit synth state to SINF, Status %d", rc) ;
            ech_tra_p_Trace ( s_FctName, s_Trace ) ;
          }
        }

      }
      else {
        sprintf(s_LoggedText,"Rtap env: %s DIF procNum: %d", r_GenData.s_RtapEnvironment,r_GenData.o_DIFprocNum);
      }
#endif

    }
    else {
      LOG(ERROR) << fname << ": Can't set site " << site->name() << " StateVal: " << _state;
    }
  }
  else {
    LOG(ERROR) << fname << ": Automate SynthStateValue: " << _state;
  }

  return rc;
}

// ==========================================================================================================
// EgsaTranslator::processing_STATE читает соответствующую секцию входных файлов с данными, а эта функция выполняет необходимые изменения
int EGSA::processing_STATE(AcqSiteEntry* site, synthstate_t i_StateValue)
{
  static const char *fname = "processing_STATE";
  int i_Status = OK;
  int i_StateStatus = OK;
  const char* state_label = SYNTHSTATE_LABELS[SYNTHSTATE_PRE_OPER + 1];
  uint32_t i_error_code;        // r_FailCause.i_error_code
  char s_error_text[80 + 1];    // r_FailCause.s_error_text
  gof_state_t e_State;
  xdb::AttributeInfo_t r_RecAttr;
  bool b_RecAttrChanged = false;
  bool b_DeleteReq = false;
  synthstate_t i_SynthState;


  // Stop initialisation command for not operationnal site
  // ----------------------------------------------------------
 

    LOG(INFO) << fname << ": " << site->name() << " site state is " << state_label << "(" << i_StateValue << ")";

    if (i_StateValue != SYNTHSTATE_OPER) {

      // Update synthetic state
      // ---------------------------
      i_Status = esg_acq_dac_SynthStateMan (site, i_StateValue);
      if (i_Status == OK) {
         e_State = GOF_D_FAILURE;
         i_error_code = 0;
         s_error_text[0] = '\0';
         b_DeleteReq = true;
      }
    }
    else {
      // Synchronisation of DIR-DIPL initialisation :
      // DIR will start init to DIPL only when DIPL has terminated init with DIR (b_DistantInitTerminated is set to TRUE)
      i_StateStatus = smed()->ech_smd_p_ReadRecAttr(site->name(),
                                            /*r_ReqResp.Provider_Id*/ site->name(),
                                            "SYNTHSTATE",
                                            &r_RecAttr,
                                            b_RecAttrChanged);
      if (i_StateStatus != OK) {
        LOG(ERROR) << fname << ": Get synth state of site " << site->name() /*r_ReqResp.Provider_Id*/
                    << " attr_chg:" << b_RecAttrChanged;
      }                
      if (i_StateStatus == OK) {
        // Set synth state from unreachable to pre-operational if response of STATECMD is OK
        // --------------------------------------------------------------------------
        if (r_RecAttr.value.fixed.val_uint32 < SYNTHSTATE_PRE_OPER)
          i_SynthState = static_cast<synthstate_t>(r_RecAttr.value.fixed.val_uint32);
        else {
          LOG(ERROR) << fname << ": unsupported " << site->name() << ".SYNTHSTATE value:" << r_RecAttr.value.fixed.val_uint32;
          i_SynthState = SYNTHSTATE_UNREACH;
        }

        if (i_SynthState == SYNTHSTATE_UNREACH) {

          LOG(INFO) << fname << ": Set synth state from UNREACH to PREOP if state req OK";

          // Удаленный Сайт сообщил, что он в онлайне. Начать инициализацию связи с ним.
          i_StateValue = SYNTHSTATE_PRE_OPER;
          i_StateStatus = esg_acq_dac_SynthStateMan (site, i_StateValue);

          if (i_StateStatus != OK) {
            LOG(ERROR) << fname << ": Update and set synth state of Site %s" << site->name() /*r_ReqResp.Provider_Id*/;
          }
        }
      }
#if 1
      LOG(INFO) << fname << ": r_LocalGenData";
#else
      esg_esg_odm_ConsultGeneralData(&r_LocalGenData);
      sprintf(s_Trace, ", Loc ReqId %d, Local Site obj type %d, b_DistantInitTerminated %d",
              r_OResultEntry.r_ManagedRequestDesc.r_ExchangedRequest.h_ReqId,
              r_LocalGenData.o_ObjectType,
              r_AcqSite.b_DistantInitTerminated);
      LOG(INFO) << fname << s_Trace;

      if ((r_OResultEntry.r_ManagedRequestDesc.r_ExchangedRequest.h_ReqId == ESG_ESG_D_LOCID_INITCOMD)
        &&(r_LocalGenData.o_ObjectType == GOF_D_BDR_OBJCLASS_DIR)
        &&(r_AcqSite.b_DistantInitTerminated == false)) {
              e_State = GOF_D_FAILURE;
              i_error_code = 0;
              s_error_text[0] = '\0';
              b_DeleteReq = true;
      }
#endif
    }

  return i_Status;
}
// ==========================================================================================================
