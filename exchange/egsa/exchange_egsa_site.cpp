#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

// Общесистемные заголовочные файлы
#include <memory>
#include <vector>
#include <map>
#include <cstring>

// Служебные заголовочные файлы сторонних утилит
#include "glog/logging.h"

// Служебные файлы RTDBUS
#include "exchange_egsa_site.hpp"
#include "exchange_egsa_impl.hpp"

#include "exchange_smad_int.hpp"
#include "exchange_config_sac.hpp"

// ==============================================================================
AcqSiteEntry::AcqSiteEntry(EGSA* egsa, const egsa_config_site_item_t* entry)
  : m_egsa(egsa),
    m_NatureAcqSite(entry->nature),
    m_AutomaticalInit(entry->auto_init),
    m_AutomaticalGenCtrl(entry->auto_gencontrol),
    m_FunctionalState(SA_STATE_UNKNOWN),
    m_Level(entry->level),
    m_InterfaceComponentActive(false),
    m_smad(NULL)
{
  std::string sa_config_filename;
  sa_common_t sa_common;
  AcquisitionSystemConfig* sa_config = NULL;

  strncpy(m_IdAcqSite, entry->name.c_str(), TAG_NAME_MAXLEN);

  // Имя СС не может содержать символа "/"
  assert(name()[0] != '/');

  sa_config_filename.assign(name());
  sa_config_filename += ".json";

  // Определить для указанной СС название файла-снимка SMAD
  sa_config = new AcquisitionSystemConfig(sa_config_filename.c_str());
  if (NOK == sa_config->load_common(sa_common)) {
     LOG(ERROR) << "Unable to parse SA " << name() << " common config";
  }
  else {
    m_smad = new InternalSMAD(sa_common.name.c_str(), sa_common.nature, sa_common.smad.c_str());

    // TODO: подключаться к InternalSMAD только после успешной инициализации модуля данной СС
#if 0
    if (STATE_OK != (m_smad->state() = m_smad->attach(name(), nature()))) {
      LOG(ERROR) << "FAIL attach to '" << name() << "', file=" << sa_common.smad
                 << ", rc=" << m_smad->state();
    }
    else {
      LOG(INFO) << "OK attach to  '" << name() << "', file=" << sa_common.smad;
    }
#endif
  }

//  m_cycles = look_my_cycles();

  delete sa_config;
}

// ==============================================================================
AcqSiteEntry::~AcqSiteEntry()
{
  delete m_smad;
}

// ==============================================================================
int AcqSiteEntry::send(int msg_id)
{
  int rc = OK;

  switch(msg_id) {
    case ADG_D_MSG_ENDALLINIT:  /*process_end_all_init();*/ break;
    case ADG_D_MSG_ENDINITACK:  /*process_end_init_acq();*/ break;
    case ADG_D_MSG_INIT:        /*process_init();        */ break;
    case ADG_D_MSG_DIFINIT:     /*process_dif_init();    */ break;
    default:
      LOG(ERROR) << "Unsupported message (" << msg_id << ") to send to " << name();
      assert(0 == 1);
      rc = NOK;
  }

  return rc;
}

// -----------------------------------------------------------------------------------
// TODO: послать сообщение об успешном завершении инициализации связи
// сразу после того, как последняя из CC успешно ответила на сообщение об инициализации
void AcqSiteEntry::process_end_all_init()
{
//1  m_timer_ENDALLINIT = new Timer("ENDALLINIT");
//  m_egsa->send_to(name(), ADG_D_MSG_ENDALLINIT);
  LOG(INFO) << "Process ENDALLINIT for " << name();
}

// -----------------------------------------------------------------------------------
// Запрос состояния завершения инициализации
void AcqSiteEntry::process_end_init_acq()
{
  LOG(INFO) << "Process ENDINITACQ for " << name();
}

// -----------------------------------------------------------------------------------
// Конец инициализации
void AcqSiteEntry::process_init()
{
  LOG(INFO) << "Process INIT for " << name();
}

// -----------------------------------------------------------------------------------
// Запрос завершения инициализации после аварийного завершения
void AcqSiteEntry::process_dif_init()
{
  LOG(INFO) << "Process DIFINIT for " << name();
}

// -----------------------------------------------------------------------------------
// TODO: послать сообщение об завершении связи
void AcqSiteEntry::shutdown()
{
//1  m_timer_INIT = new Timer("SHUTDOWN");
  LOG(INFO) << "Process SHUTDOWN for " << name();
}

// -----------------------------------------------------------------------------------
int AcqSiteEntry::control(int code)
{
  int rc = NOK;

  // TODO: Передать интерфейсному модулю указанную команду
  LOG(INFO) << "Control SA '" << name() << "' with " << code << " code";
  return rc;
}

// -----------------------------------------------------------------------------------
// Прочитать изменившиеся данные
int AcqSiteEntry::pop(sa_parameters_t&)
{
  int rc = NOK;
  LOG(INFO) << "Pop changed data from SA '" << name() << "'";
  return rc;
}

// -----------------------------------------------------------------------------------
// Передать в СС указанные значения
int AcqSiteEntry::push(sa_parameters_t&)
{
  int rc = NOK;
  LOG(INFO) << "Push data to SA '" << name() << "'";
  return rc;
}

// -----------------------------------------------------------------------------------
int AcqSiteEntry::ask_ENDALLINIT()
{
  LOG(INFO) << "CALL AcqSiteEntry::ask_ENDALLINIT() for " << name();
  return NOK;
}
// -----------------------------------------------------------------------------------
void AcqSiteEntry::check_ENDALLINIT()
{
  LOG(INFO) << "CALL AcqSiteEntry::check_ENDALLINIT() for " << name();
}

// ==============================================================================
// TODO: СС и EGSA могут работать на разных хостах, в этом случае подключение EGSA к smad СС
// не получит доступа к реальным данным от СС. Их придется EGSA туда заносить самостоятельно.
int AcqSiteEntry::attach_smad()
{
#if 0
    if (STATE_OK != (m_smad->state() = m_smad->attach(name(), nature()))) {
      LOG(ERROR) << "FAIL attach to '" << name() << "', file=" << sa_common.smad
                 << ", rc=" << m_smad->state();
    }
    else {
      LOG(INFO) << "OK attach to  '" << name() << "', file=" << sa_common.smad;
    }
#endif

  return NOK;
}

// ==============================================================================
int AcqSiteEntry::detach_smad()
{
  int rc = NOK;

  LOG(WARNING) << "Not implemented";

  return rc;
}

// ==============================================================================
#warning "Продолжить тут - Реализовать машину состояний CC"
// Изменение состояния СС вызывают изменения в очереди Запросов
// TODO: Реализовать машину состояний
sa_state_t AcqSiteEntry::change_state_to(sa_state_t new_state)
{
  bool state_changed = false;

  switch (m_FunctionalState) {  // Старое состояние
    // --------------------------------------------------------------------------
    case SA_STATE_UNKNOWN:  // Неопределено
    // --------------------------------------------------------------------------

      switch(new_state) {
        // ||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||
        case SA_STATE_UNREACH:
          state_changed = true;
          // TODO: удалить возможно оставшиеся в очереди Запросы
        case SA_STATE_UNKNOWN:
          m_FunctionalState = new_state;
          break;
        // ||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||
        case SA_STATE_OPER:
          state_changed = true;
          // TODO: активировать запросы данных для этой СС
          // activate_cycle()
          break;
        // ||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||
        case SA_STATE_PRE_OPER:
          state_changed = true;
          // TODO: активировать запросы на инициализацию связи с этой СС
          break;
        // ||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||
        case SA_STATE_INHIBITED:
        case SA_STATE_FAULT:
        case SA_STATE_DISCONNECTED:
          state_changed = true;
          // TODO: удалить возможно оставшиеся в очереди Запросы
          break;
        // ||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||
      }

      break;

    // --------------------------------------------------------------------------
    case SA_STATE_UNREACH:  // Недоступна
    // --------------------------------------------------------------------------
      switch(new_state) {
        // ||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||
        case SA_STATE_UNREACH:
          state_changed = true;
          // TODO: удалить возможно оставшиеся в очереди Запросы
        case SA_STATE_UNKNOWN:
          m_FunctionalState = new_state;
          break;
        // ||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||
        case SA_STATE_OPER:
          state_changed = true;
          // TODO: активировать запросы данных для этой СС
          // activate_cycle()
          break;
        // ||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||
        case SA_STATE_PRE_OPER:
          state_changed = true;
          // TODO: активировать запросы на инициализацию связи с этой СС
          break;
        // ||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||
        case SA_STATE_INHIBITED:
        case SA_STATE_FAULT:
        case SA_STATE_DISCONNECTED:
          state_changed = true;
          // TODO: удалить возможно оставшиеся в очереди Запросы
          break;
        // ||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||

      }
      break;

    // --------------------------------------------------------------------------
    case SA_STATE_OPER:     // Оперативная работа
    // --------------------------------------------------------------------------
      switch(new_state) {
        case SA_STATE_UNKNOWN: break;
        case SA_STATE_UNREACH: break;
        case SA_STATE_OPER: break;
        case SA_STATE_PRE_OPER: break;
        case SA_STATE_INHIBITED: break;
        case SA_STATE_FAULT: break;
        case SA_STATE_DISCONNECTED: break;
      }
      break;

    // --------------------------------------------------------------------------
    case SA_STATE_PRE_OPER: // В процессе инициализации
    // --------------------------------------------------------------------------
      switch(new_state) {
        case SA_STATE_UNKNOWN: break;
        case SA_STATE_UNREACH: break;
        case SA_STATE_OPER: break;
        case SA_STATE_PRE_OPER: break;
        case SA_STATE_INHIBITED: break;
        case SA_STATE_FAULT: break;
        case SA_STATE_DISCONNECTED: break;
      }
      break;

    // --------------------------------------------------------------------------
    case SA_STATE_INHIBITED:// в запрете работы
    // --------------------------------------------------------------------------
      switch(new_state) {
        case SA_STATE_UNKNOWN: break;
        case SA_STATE_UNREACH: break;
        case SA_STATE_OPER: break;
        case SA_STATE_PRE_OPER: break;
        case SA_STATE_INHIBITED: break;
        case SA_STATE_FAULT: break;
        case SA_STATE_DISCONNECTED: break;
      }

      break;

    // --------------------------------------------------------------------------
    case SA_STATE_FAULT:    // сбой
    // --------------------------------------------------------------------------
      switch(new_state) {
        case SA_STATE_UNKNOWN: break;
        case SA_STATE_UNREACH: break;
        case SA_STATE_OPER: break;
        case SA_STATE_PRE_OPER: break;
        case SA_STATE_INHIBITED: break;
        case SA_STATE_FAULT: break;
        case SA_STATE_DISCONNECTED: break;
      }
      break;

    // --------------------------------------------------------------------------
    case SA_STATE_DISCONNECTED: // не подключена
    // --------------------------------------------------------------------------
      switch(new_state) {
        case SA_STATE_UNKNOWN: break;
        case SA_STATE_UNREACH: break;
        case SA_STATE_OPER: break;
        case SA_STATE_PRE_OPER: break;
        case SA_STATE_INHIBITED: break;
        case SA_STATE_FAULT: break;
        case SA_STATE_DISCONNECTED: break;
      }
      break;
  }

  if (state_changed) {
    LOG(INFO) << name() << ": state was changed from " << m_FunctionalState << " to " << new_state;
  }
  m_FunctionalState = new_state;
  return m_FunctionalState;
}

// ==============================================================================
// Запомнить связку между Циклом и последовательностью Запросов
// NB: некоторые вложенные запросы могут быть отменены. Критерий отмены - отсутствие
// параметров, собираемых этим запросом.
// К примеру, виды собираемой от СС информации по Запросам:
//      Тревоги     : A_URGINFOS
//      Телеметрия  : A_INFOSACQ
//      Телеметрия? : A_GCPRIMARY | A_GCSECOND | A_GCTERTIARY
// Виды передаваемой в адрес СС информации по Запросам
//      Телеметрия  : A_IAPRIMARY | A_IASECOND | A_IATERTIARY
//      Уставки     : A_ALATHRES
int AcqSiteEntry::push_request_for_cycle(Cycle* cycle, int* included_requests)
{
  int rc = OK;
  int ir = 0;
  char tmp[200] = " ";
  char s_req[20];

  for (int idx = 0; idx < NBREQUESTS; idx++) {
    if (included_requests[idx]) {
      ir++;
      strcpy(s_req, Request::name(static_cast<ech_t_ReqId>(idx)));
      strcat(tmp, s_req);
      strcat(tmp, " ");
    }
  }

  // Игнорировать Cycle->req_id(), если included_requests не пуст
  if (!ir) { // Нет вложенных Подзапросов - используем сам Запрос
    strcat(tmp, Request::name(cycle->req_id()));
    strcat(tmp, " ");
  }

  LOG(INFO) << name() << ": store request(s) [" << tmp << "] for cycle " << cycle->name();
  return rc;
}

// ==============================================================================
AcqSiteList::AcqSiteList()
 : m_egsa(NULL)
{
}

// ==============================================================================
AcqSiteList::~AcqSiteList()
{
  // TODO Разные экземпляры AcqSiteList в Cycle и в EGSA содержат ссылки на одни
  // и те же экземпляры объектов AcqSiteEntry
  for (std::vector<std::shared_ptr<AcqSiteEntry*>>::iterator it = m_items.begin();
       it != m_items.end();
       ++it)
  {
    // Удалить ссылку на AcqSiteEntry
    (*it).reset();
  }
}

// ==============================================================================
void AcqSiteList::set_egsa(EGSA* egsa)
{
  m_egsa = egsa;
}

// ==============================================================================
int AcqSiteList::attach_to_smad(const char* sa_name)
{
  return NOK;
}

// ==============================================================================
int AcqSiteList::detach()
{
  int rc = NOK;

  // TODO: Для всех подчиненных систем сбора:
  // 1. Изменить их состояние SYNTHSTATE на "ОТКЛЮЧЕНО"
  // 2. Отключиться от их внутренней SMAD
  for(std::vector<std::shared_ptr<AcqSiteEntry*>>::const_iterator it = m_items.begin();
      it != m_items.end();
      ++it)
  {
    AcqSiteEntry* entry = *(*it);
    LOG(INFO) << "TODO: set " << entry->name() << ".SYNTHSTATE = 0";
    LOG(INFO) << "TODO: detach " << entry->name() << " SMAD";
  }

  return rc;
}

// ==============================================================================
int AcqSiteList::detach_from_smad(const char* name)
{
  int rc = NOK;

  LOG(INFO) << "TODO: detach " << name << " SMAD";
  return rc;
}

// ==============================================================================
void AcqSiteList::insert(AcqSiteEntry* the_new_one)
{
  m_items.push_back(std::make_shared<AcqSiteEntry*>(the_new_one));
}

// ==============================================================================
// Вернуть элемент по имени Сайта
AcqSiteEntry* AcqSiteList::operator[](const char* name)
{
  AcqSiteEntry *entry = NULL;

  for(std::vector< std::shared_ptr<AcqSiteEntry*> >::const_iterator it = m_items.begin();
      it != m_items.end();
      ++it)
  {
    if (0 == std::strcmp((*(*it))->name(), name)) {
      entry = *(*it);
      break;
    }
  }

  return entry;
}

// ==============================================================================
// Вернуть элемент по имени Сайта
AcqSiteEntry* AcqSiteList::operator[](const std::string& name)
{
  AcqSiteEntry *entry = NULL;

  for(std::vector<std::shared_ptr<AcqSiteEntry*>>::const_iterator it = m_items.begin();
      it != m_items.end();
      ++it)
  {
    if (0 == name.compare((*(*it))->name())) {
      entry = *(*it);
      break;
    }
  }

  return entry;
}

// ==============================================================================
// Вернуть элемент по индексу
AcqSiteEntry* AcqSiteList::operator[](const std::size_t idx)
{
  AcqSiteEntry *entry = NULL;

  if (idx < m_items.size()) {
    entry = *m_items.at(idx);
  }

  return entry;
}

// ==============================================================================
// Освободить все ресурсы
int AcqSiteList::release()
{
  for (std::vector<std::shared_ptr<AcqSiteEntry*>>::iterator it = m_items.begin();
       it != m_items.end();
       ++it)
  {
    LOG(INFO) << "release site " << (*(*it))->name();
    AcqSiteEntry *entry = *(*it); 
    (*it).reset();
    delete entry;
  }
  m_site_map.clear();

  return OK;
}
// ==============================================================================
