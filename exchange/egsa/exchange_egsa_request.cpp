#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

// Общесистемные заголовочные файлы
#include <assert.h>
#include <limits.h>

// Служебные заголовочные файлы сторонних утилит
#include "glog/logging.h"

// Служебные файлы RTDBUS
#include "exchange_config_egsa.hpp"
#include "exchange_egsa_request.hpp"
#include "exchange_egsa_cycle.hpp"
#include "exchange_egsa_site.hpp"

size_t Request::m_sequence = 0;

// Вспомогательный словарь для получения соответствия между идентификатором запроса
// (порядковым номером) и символьным описанием (элементом массива)
const char* Request::m_dict_RequestNames[] = {
  EGA_EGA_D_STRGENCONTROL,
  EGA_EGA_D_STRINFOSACQ,
  EGA_EGA_D_STRURGINFOS,
  EGA_EGA_D_STRGAZPROCOP,
  EGA_EGA_D_STREQUIPACQ,
  EGA_EGA_D_STRACQSYSACQ,
  EGA_EGA_D_STRALATHRES,
  EGA_EGA_D_STRTELECMD,
  EGA_EGA_D_STRTELEREGU,
  EGA_EGA_D_STRSERVCMD,
  EGA_EGA_D_STRGLOBDWLOAD,
  EGA_EGA_D_STRPARTDWLOAD,
  EGA_EGA_D_STRGLOBUPLOAD,
  EGA_EGA_D_STRINITCMD,
  EGA_EGA_D_STRGCPRIMARY,
  EGA_EGA_D_STRGCSECOND,
  EGA_EGA_D_STRGCTERTIARY,
  EGA_EGA_D_STRIAPRIMARY,
  EGA_EGA_D_STRIASECOND,
  EGA_EGA_D_STRIATERTIARY,
  EGA_EGA_D_STRINFOSDIFF,
  EGA_EGA_D_STRDELEGATION,
  EGA_EGA_D_STRNOEXISTENT };

// ==============================================================================
// НСИ
Request::Request(const ega_ega_odm_t_RequestEntry* _config)
  : mtx(),
    m_internal_dump(),
    m_config(),
    m_when(),
    m_trigger_callback(),
    m_exchange_id(),
    
    m_last_in_bundle(false),
    m_site(NULL),
    m_cycle(NULL)
{
  const char* name = ((_config)? _config->s_RequestName : "<empty>");
  if (!_config) {
    memset(&m_config, '\0', sizeof(ega_ega_odm_t_RequestEntry));
  }
  else {
    memcpy(&m_config, _config, sizeof(ega_ega_odm_t_RequestEntry));
  }
  generate_new_exchange_id();
  m_trigger_callback = std::bind(&Request::trigger, this);
  LOG(INFO) << "CTOR Request " << name << " ega_ega_odm_t_RequestEntry "
            << m_exchange_id
            << " (" << m_dict_RequestNames[m_config.e_RequestId] << ")";
}

#if 0
Request::Request(const callback_type &cb, const time_t &when)
  : m_config()
//  : m_trigger_callback(cb)
{
//1  memset(&m_config, '\0', sizeof(ega_ega_odm_t_RequestEntry));
  m_when = std::chrono::system_clock::from_time_t(when);
  generate_new_exchange_id();
  m_trigger_callback = std::bind(&Request::trigger, this);
  LOG(INFO) << "CTOR Request time_t " << m_exchange_id;
}

Request::Request(const callback_type &cb, const timeval &when)
  : m_config()
{
  m_when = std::chrono::system_clock::from_time_t(when.tv_sec) +
                       std::chrono::microseconds(when.tv_usec);
//1  memset(&m_config, '\0', sizeof(ega_ega_odm_t_RequestEntry));
  generate_new_exchange_id();
  m_trigger_callback = std::bind(&Request::trigger, this);
  LOG(INFO) << "CTOR Request timeval " << m_exchange_id;
}

Request::Request(const callback_type& cb, const std::chrono::time_point<std::chrono::system_clock> &when)
  : m_config(),
    m_when(when),
    m_trigger_callback(cb)
{
  generate_new_exchange_id();
  LOG(INFO) << "CTOR Request chrono " << m_exchange_id;
}
#endif

Request::Request(const Request& orig)
  : mtx(),
    m_internal_dump(),
    m_config(orig.m_config),
    m_when(orig.m_when),
    m_trigger_callback(),
    m_exchange_id(orig.m_exchange_id),
    m_last_in_bundle(orig.m_last_in_bundle),
    m_site(orig.m_site),
    m_cycle(orig.m_cycle)
{
  generate_new_exchange_id();
  m_trigger_callback = std::bind(&Request::trigger, this);
  LOG(INFO) << "CTOR Request Request& " << m_exchange_id;
}

Request::Request(const Request* orig)
  : mtx(),
    m_internal_dump(),
    m_config(orig->m_config),
    m_when(orig->m_when),
    m_exchange_id(orig->m_exchange_id),
    m_last_in_bundle(orig->m_last_in_bundle),
    m_site(orig->m_site),
    m_cycle(orig->m_cycle)
{
  generate_new_exchange_id();
  m_trigger_callback = std::bind(&Request::trigger, this);
  LOG(INFO) << "CTOR Request Request* " << m_exchange_id;
}

Request& Request::operator=(const Request& orig)
{
  memcpy(&m_config, &orig.m_config, sizeof(ega_ega_odm_t_RequestEntry));
  m_when = orig.m_when;
  m_exchange_id = orig.m_exchange_id;
  m_last_in_bundle = false;
  m_site = orig.m_site;
  m_cycle = orig.m_cycle;

  generate_new_exchange_id();
  m_trigger_callback = std::bind(&Request::trigger, this);
  LOG(INFO) << "operator= Request " << m_exchange_id;
  return *this;
}

// ==============================================================================
size_t Request::generate_new_exchange_id()
{
  mtx.lock();

  if (m_sequence >= ULONG_MAX - 1)
    m_sequence = 0;

  m_exchange_id = m_sequence++;

  mtx.unlock();
  return m_exchange_id;
}

// ==============================================================================
// Используется для хранения динамической информации в процессе работы
Request::Request(const Request* _req, AcqSiteEntry* _site, Cycle* _cycle)
  : m_config(_req->m_config),
    m_when(_req->m_when),
    m_exchange_id(_req->m_exchange_id),
    m_site(_site),
    m_cycle(_cycle)
{
}

// ==============================================================================
Request::~Request()
{
  LOG(INFO) << "release request exchange:" << m_exchange_id << " id:" << m_config.e_RequestId;
}

// ==============================================================================
// TODO: Вероятно, не стоит давать возможность Запросу самоповторяться
int Request::trigger()
{
  const int REQUEST_ID_LAST = ECH_D_NOT_EXISTENT + 1;
  const int SA_STATE_LAST = SA_STATE_UNKNOWN + 1;
  //      0
  //      |                 21
  //      |                 |                 0
  //      |                 |                 |                 6
  //      |                 |                 |                 |
  //bool [ECH_D_GENCONTROL..ECH_D_DELEGATION][SA_STATE_UNREACH..SA_STATE_UNKNOWN] = {
  // Номер строки - тип текущего Запроса
  // Номер столбца - состояние связанной с Запросом СС
#define X true
#define _ false
  static const bool enabler_matrix[REQUEST_ID_LAST][SA_STATE_LAST] = {
    //                         UNREACH
    //                         |  OPER
    //                         |  |  PRE_OPER
    //                         |  |  |  INHIBITED
    //                         |  |  |  |  FAULT
    //                         |  |  |  |  |  DISCONNECTED
    //                         |  |  |  |  |  |  UNKNOWN
    //                         |  |  |  |  |  |  |
    /* ECH_D_GENCONTROL */   { _, X, X, _, _, _, _ },
    /* ECH_D_INFOSACQ   */   { _, X, X, _, _, _, _ },
    /* ECH_D_URGINFOS   */   { _, X, X, _, _, _, _ },
    /* ECH_D_GAZPROCOP  */   { _, _, _, _, _, _, _ },
    /* ECH_D_EQUIPACQ   */   { _, X, X, X, X, _, _ },
    /* ECH_D_ACQSYSACQ  */   { _, X, X, X, X, X, X },
    /* ECH_D_ALATHRES   */   { _, X, X, _, _, _, _ },
    /* ECH_D_TELECMD    */   { _, X, _, _, _, _, _ },
    /* ECH_D_TELEREGU   */   { _, X, _, _, _, _, _ },
    /* ECH_D_SERVCMD    */   { X, X, X, _, X, _, _ },
    /* ECH_D_GLOBDWLOAD */   { _, X, _, _, _, _, _ },
    /* ECH_D_PARTDWLOAD */   { _, X, _, _, _, _, _ },
    /* ECH_D_GLOBUPLOAD */   { _, X, _, _, _, _, _ },
    /* ECH_D_INITCMD    */   { X, X, X, _, X, X, X },
    /* ECH_D_GCPRIMARY  */   { _, X, _, _, _, _, _ },
    /* ECH_D_GCSECOND   */   { _, X, _, _, _, _, _ },
    /* ECH_D_GCTERTIARY */   { _, X, _, _, _, _, _ },
    /* ECH_D_DIFFPRIMARY*/   { _, X, _, _, _, _, _ },
    /* ECH_D_DIFFSECOND */   { _, X, _, _, _, _, _ },
    /* ECH_D_DIFFTERTIARY */ { _, X, _, _, _, _, _ },
    /* ECH_D_INFODIFFUSION*/ { _, X, _, _, _, _, _ },
    /* ECH_D_DELEGATION   */ { _, X, X, _, _, X, _ },
  };
#undef _
#undef X
  int rc = ONCE;

  // Проверить, разрешен ли Запрос для текущего состояния СС
  if (enabler_matrix[m_config.e_RequestId][m_site->state()]) {
    // Да, разрешен
    LOG(INFO) << "TRIGGER REQ [" << dump() << "] enabler:" << enabler_matrix[m_config.e_RequestId][m_site->state()];

    // TODO: Выполнить действия, предполагаемые данным запросом:
    //  * прочитать данные из SMAD и передать их в БДРВ для Запросов типов Сбора Данных (GENCONTROL, URGINFOS,...)
    //  * получить состояние СС для Запросов типа Управление (ACQSYSACQ, SERVCMD,...)
    //  * ...
    //
    m_when += std::chrono::seconds(1); // GEV: повторять запрос каждую секунду
    rc = REPEAT;
  }
  else {
    // Нет, запрещен
  }
  
  /*
  switch (m_site->state()) {
    case 
  }*/

#if 0
  // Запросы, связанные с обменом информацией, возможны только при оперативном состоянии СС
  // Запросы, связанные с попытками подключения к СС, возможны при любом состоянии СС
  switch (m_config.e_RequestId) {
    case ECH_D_GENCONTROL:  = 0,   /* general control */
    case ECH_D_INFOSACQ     = 1,   /* global acquisition */
    case ECH_D_URGINFOS     = 2,   /* urgent data acquisition */
    case ECH_D_GAZPROCOP    = 3,   /* gaz volume count acquisition */
    case ECH_D_EQUIPACQ     = 4,   /* equipment acquisition */
    case ECH_D_ACQSYSACQ    = 5,   /* Sac data acquisition */
    case ECH_D_ALATHRES     = 6,   /* alarms and thresholds acquisition */
    case ECH_D_TELECMD      = 7,   /* telecommand */
    case ECH_D_TELEREGU     = 8,   /* teleregulation */
    case ECH_D_SERVCMD      = 9,   /* service command */
    case ECH_D_GLOBDWLOAD   = 10,  /* global download */
    case ECH_D_PARTDWLOAD   = 11,  /* partial download */
    case ECH_D_GLOBUPLOAD   = 12,  /* Sac configuration global upload */
    case ECH_D_INITCMD      = 13,  /* initialisation of the exchanges */
    case ECH_D_GCPRIMARY    = 14,  /* Primary general control */
    case ECH_D_GCSECOND     = 15,  /* Secondary general control */
    case ECH_D_GCTERTIARY   = 16,  /* Tertiary general control */
    case ECH_D_DIFFPRIMARY  = 17,  /* Primary differential acquisition */
    case ECH_D_DIFFSECOND   = 18,  /* Secondary differential acquisition */
    case ECH_D_DIFFTERTIARY = 19,  /* Tertiary differential acquisition */
    case ECH_D_INFODIFFUSION= 20,  /* Information diffusion              */
    case ECH_D_DELEGATION   = 21,  /* Process order delegation-end of delegation */
    case ECH_D_NOT_EXISTENT = 22   /* request not exist */
  }

  switch (m_site->state()) {
    case SA_STATE_UNKNOWN:
      break;
    case SA_STATE_UNREACH:
      break;
    case SA_STATE_OPER:
      break;
    case SA_STATE_PRE_OPER:
      break;
    case SA_STATE_INHIBITED:
      break;
    case SA_STATE_FAULT:
      break;
    case SA_STATE_DISCONNECTED:
      break;
  }
#else
#endif

  return rc;
}

// ==============================================================================
// Строка с характеристиками Запроса
const char* Request::dump()
{
  snprintf(m_internal_dump, DUMP_SIZE, "name:%s prio:%d id:%d last:%d site:%s (state:%d) cycle:%s exch:%d when:%ld mode:%d object:%d",
           name(),
           priority(),
           id(),
           m_last_in_bundle,
           ((m_site)? m_site->name() : "NULL"),
           ((m_site)? m_site->state() : SA_STATE_UNKNOWN),
           ((m_cycle)? m_cycle->name() : "NULL"),
           exchange_id(),
           std::chrono::system_clock::to_time_t(when()),
           acq_mode(),
           objclass());
  return m_internal_dump;
}

// ==============================================================================
RequestDictionary::RequestDictionary()
{
  m_requests.clear();
}

// ==============================================================================
RequestDictionary::~RequestDictionary()
{
  for(std::map<ech_t_ReqId, Request*>::const_iterator it = m_requests.begin();
      it != m_requests.end();
      ++it)
  {
    delete (*it).second;
  }

  m_requests.clear();
}

// ==============================================================================
int RequestDictionary::add(Request* item)
{
  m_requests.insert(std::pair<ech_t_ReqId, Request*>(item->id(), item));
  return NOK;
}

// ==============================================================================
void RequestDictionary::release()
{
  for(std::map<ech_t_ReqId, Request*>::iterator it = m_requests.begin();
      it != m_requests.end();
      ++it)
  {
    LOG(INFO) << "Free dictionary req " << (*it).second->name();
    delete (*it).second;
  }
}

// ==============================================================================
// Вернуть экземпляр Request соответствующего типа
Request* RequestDictionary::query_by_id(ech_t_ReqId _id)
{
  std::map<ech_t_ReqId, Request*>::iterator it;
  Request* item = NULL;

  if (m_requests.end() != (it = m_requests.find(_id))) {
    item = (*it).second;
    assert(item);
    LOG(INFO) << "query_by_id(" << _id << ")=" << item->name();
  }
  else LOG(ERROR) << "Can't locate request " << _id << " from dictionary";

  return item;
}

// ==============================================================================
RequestRuntimeList::RequestRuntimeList()
{
  LOG(INFO) << "CTOR RequestRuntimeList";
}

// ==============================================================================
RequestRuntimeList::~RequestRuntimeList()
{
  LOG(INFO) << "DTOR RequestRuntimeList";
  while (!m_request_queue.empty())
  {
      LOG(INFO) << "REQ QUEUE TOP: " << m_request_queue.top().name();
      m_request_queue.pop();

     /* if (!queue.empty())
      {
        LOG(INFO) << ", ";
      }*/
  }
}

// ==============================================================================
void RequestRuntimeList::add(Request* req, const time_t &delta)
{
  const time_t when = time(0) + delta;
  auto real_when = std::chrono::system_clock::from_time_t(when);

  LOG(INFO) << "ARM " << req->name() << " trigger for " << delta << " sec";
  req->arm(real_when);
 //1 add(std::bind(&Request::trigger, req), when);
  m_request_queue.emplace(req);
}

#if 0
// ==============================================================================
void RequestRuntimeList::add(const callback_type &cb, const time_t &when)
{
  auto real_when = std::chrono::system_clock::from_time_t(when);

  m_request_queue.emplace(cb, real_when);
}

// ==============================================================================
void RequestRuntimeList::add(const callback_type &cb, const timeval &when)
{
  auto real_when = std::chrono::system_clock::from_time_t(when.tv_sec) +
                   std::chrono::microseconds(when.tv_usec);

  m_request_queue.emplace(cb, real_when);
}

// ==============================================================================
void RequestRuntimeList::add(const callback_type &cb,
         const std::chrono::time_point<std::chrono::system_clock> &when)
{
  m_request_queue.emplace(cb, when);
}
#endif

// ==============================================================================
void RequestRuntimeList::clear()
{
  while (!m_request_queue.empty())
  {
    m_request_queue.pop();
  }
}

// ==============================================================================
bool RequestRuntimeList::remove(const Request& re) { /*
    auto it = std::find(m_request_queue.begin(), m_request_queue.end(), ev);
    if (it != m_request_queue.end()) {
          m_request_queue.erase(it);
          std::make_heap(m_request_queue.begin(), m_request_queue.end(), event_less);
          return true;
    }
    else */{
      return false;
    }
}

// ==============================================================================
// Итерация по выборке подходящих Запросов из сортированной очереди
void RequestRuntimeList::timer()
{
      time_type now = std::chrono::system_clock::now();

      while (!m_request_queue.empty() &&
             (m_request_queue.top().when() < now))
      {
        const Request& req = m_request_queue.top();

        // Если запрос говорит, что его нужно повторить - поместим обратно в очередь
        // NB: при этом новое время активации Запроса должно быть уже изменено в callback()-е
        if (Request::REPEAT == req.callback()) {
          m_request_queue.emplace(req);
        }

        m_request_queue.pop();
      }
}

// ==============================================================================
