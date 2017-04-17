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
    entry = *(*it);
    if (0 == std::strcmp(entry->name(), name))
      break;
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
    entry = *(*it);
    if (0 == name.compare(entry->name()))
      break;
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
