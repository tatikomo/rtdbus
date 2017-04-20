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
#include "exchange_egsa_sa.hpp"

// ==============================================================================
AcqSiteEntry::AcqSiteEntry(EGSA* egsa, const egsa_config_site_item_t* entry)
  : m_egsa(egsa),
    m_NatureAcqSite(entry->nature),
    m_AutomaticalInit(entry->auto_init),
    m_AutomaticalGenCtrl(entry->auto_gencontrol),
    m_FunctionalState(SA_STATE_UNKNOWN),
    m_Level(entry->level),
    m_InterfaceComponentActive(false),
    m_sa_instance(NULL)
{
  strncpy(m_IdAcqSite, entry->name.c_str(), TAG_NAME_MAXLEN);
}

// ==============================================================================
AcqSiteEntry::~AcqSiteEntry()
{
  delete m_sa_instance;
}

// ==============================================================================
// TODO: СС и EGSA могут работать на разных хостах, в этом случае подключение EGSA к smad СС
// не получит доступа к реальным данным от СС. Их придется EGSA туда заносить самостоятельно.
int AcqSiteEntry::attach_smad()
{
  m_sa_instance = new SystemAcquisition(m_egsa, this);

  return OK;
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

#warning "Игнорировать Cycle->req_id(), если included_requests не пуст"
  for (int idx = 0; idx < NBREQUESTS; idx++) {
    if (included_requests[idx]) {
      ir++;
      strcpy(s_req, Request::name(static_cast<ech_t_ReqId>(idx)));
      strcat(tmp, s_req);
      strcat(tmp, " ");
    }
  }

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
