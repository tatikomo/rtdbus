#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

// Общесистемные заголовочные файлы
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
{
}

// ==============================================================================
AcqSiteList::~AcqSiteList()
{
  for (std::vector<AcqSiteEntry*>::iterator it = m_items.begin();
       it != m_items.end();
       ++it)
  {
    LOG(INFO) << "free site " << (*it)->name();
    delete (*it);
  }
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
  for(std::vector<AcqSiteEntry*>::const_iterator it = m_items.begin();
      it != m_items.end();
      ++it)
  {
    LOG(INFO) << "TODO: set " << (*it)->name() << ".SYNTHSTATE = 0";
    LOG(INFO) << "TODO: detach " << (*it)->name() << " SMAD";
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
  // Занести новый элемент в массив, и запомнить его индекс
  const int new_item_idx = m_items.size();

  m_items.push_back(the_new_one);
  // Связать название СС и полученный недавно индекс
  m_site_map[the_new_one->name()] = new_item_idx;
}

// ==============================================================================
// Вернуть элемент по имени Сайта
AcqSiteEntry* AcqSiteList::operator[](const char* name)
{
  size_t idx = m_site_map[name];
  AcqSiteEntry* entry = m_items.at(idx);
  return entry;
}

// ==============================================================================
// Вернуть элемент по индексу
AcqSiteEntry* AcqSiteList::operator[](const std::size_t idx)
{
  if (idx < m_items.size())
    return m_items.at(idx);
  else
    return NULL;
}

// ==============================================================================
// Освободить все ресурсы
int AcqSiteList::release()
{
  for (std::vector<AcqSiteEntry*>::iterator it = m_items.begin();
       it != m_items.end();
       ++it)
  {
    LOG(INFO) << "release site " << (*it)->name();
    delete (*it);
  }
  m_site_map.clear();
}
// ==============================================================================
