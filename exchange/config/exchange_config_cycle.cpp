#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

// Общесистемные заголовочные файлы
#include <assert.h>
#include <unistd.h>
#include <linux/limits.h>
#include <list>

// Служебные заголовочные файлы сторонних утилит
#include "glog/logging.h"

// Служебные файлы RTDBUS
#include "exchange_config_site.hpp"
#include "exchange_config_cycle.hpp"

#if 0
// ===================================================================================================
CycleTrigger::CycleTrigger(const std::string& cycle_name)
  : m_cycle_name(cycle_name)
{
//  LOG(INFO) << "New trigger for " << m_cycle_name << ", no descriptor";
}

// ===================================================================================================
CycleTrigger::CycleTrigger(const std::string& cycle_name, int fd)
  : m_cycle_name(cycle_name),
    m_fd(fd)
{
//  LOG(INFO) << "New trigger for " << m_cycle_name << ", fd=" << (int)m_fd;
}

// ===================================================================================================
void CycleTrigger::handlerFunction( void )
{
  ssize_t ssz;

  LOG(INFO) << "time handler invoked: " << m_cycle_name << " to fd=" << (int)m_fd;

  if (m_cycle_name.size() >= PIPE_BUF) {
    LOG(WARNING) << "Message size (" << m_cycle_name.size() << ") is exceed atomic buffer size:" << PIPE_BUF;
  }

  if (-1 == (ssz = write(m_fd, m_cycle_name.c_str(), m_cycle_name.size()))) {
    LOG(ERROR) << "write to " << m_fd <<": " << strerror(errno);
  }
  else if (ssz < m_cycle_name.size()) {
    LOG(ERROR) << "write less (" << ssz << ") than " << m_cycle_name.size() << " bytes to fd="
               << m_fd <<": " << strerror(errno);
  }
}
#endif

// ===================================================================================================
// Конструктор экземпляра "Цикл"
// Все параметры проверяются на корректность при чтении из конфигурационного файла
// _name    : название Цикла, читается из конфигурации
// _period  : интервалы между активациями
// _id      : уникальный числовой идентификатор Цикла
// _family  : тип Цикла
Cycle::Cycle(const char* _name, int _period, cycle_id_t _id, ech_t_ReqId _req_id, cycle_family_t _family)
 : m_CycleFamily(_family),
   m_CyclePeriod(_period),
   m_CycleId(_id),
   m_RequestId(_req_id)
{
  strncpy(m_CycleName, _name, EGA_EGA_D_LGCYCLENAME);
  m_CycleName[EGA_EGA_D_LGCYCLENAME] = '\0';
}

void Cycle::dump()
{
  std::cout << "Cycle name:" << m_CycleName << " family:" << (int)m_CycleFamily
            << " period:" << (int)m_CyclePeriod << " id:" << (int)m_CycleId << " ";
  if (m_SiteList.size()) {
    std::cout << "sites: " << m_SiteList.size() << " [";
    for (size_t sid = 0; sid < m_SiteList.size(); sid++) {
      std::cout << " " << m_SiteList.at(sid)->name();
    }
    std::cout << " ]";
  }
  std::cout << std::endl;
}

#if 0
// ===================================================================================================
// Активировать цикл, взведя его таймер
int Cycle::activate(int fd)
{
  m_CycleTrigger = new CycleTrigger(m_CycleName, fd);

  LOG(INFO) << "Activate timer for cycle " << m_CycleName << " with period=" << m_CyclePeriod;

  m_CycleTimer = new Timer(m_CycleTrigger);

  m_CycleTimer->setDuration(m_CyclePeriod);
  m_CycleTimer->start();

  return OK;
}

// ===================================================================================================
// Деактивировать цикл
int Cycle::deactivate()
{
  LOG(INFO) << "Deactivate timer for cycle " << m_CycleName << " with period=" << m_CyclePeriod;
  m_CycleTimer->stop();
  return OK;
}

// ===================================================================================================
// Проверить, зарегистрирована ли указанная СС в данном Цикле
bool Cycle::exist_for_SA(const std::string& sa_name)
{
  bool found = false;

  // Ищем, если есть список сайтов не пуст
  if (site_list.size() > 0) {
    // Проверить все связанные СС
    for (std::vector<AcqSiteEntry*>::const_iterator sit = m_AcqSites.begin();
         sit != m_AcqSites.end();
         ++sit) {
      // Если найдена СС, имеющая этот цикл
      if (0 == sa_name.compare((*sit)->name())) {
        LOG(INFO) << "found cycle: " << m_CycleName << " for " << sa_name;
        found = true;
        break;
      }
    }
  }

  return found;
}
#endif

// ===================================================================================================
// Зарегистрировать указанную СС в этом Цикле
int Cycle::link(AcqSiteEntry* site)
{
  int rc = NOK;
  bool stop_it = false;

  // Добавить Сайт, если ранее он ещё не был добавлен
  for (size_t cid = 0; cid < m_SiteList.size(); cid++) {
//  std::list<AcqSiteEntry*>::iterator it = m_SiteList.begin(); it != m_SiteList.end(); ++it) {
    if (0 == strncmp(m_SiteList.at(cid)->name(), site->name(), TAG_NAME_MAXLEN)) {
      LOG(WARNING) << "Try to register in " << m_CycleName << " already known SA: " << site->name();
      stop_it = true;
      break;
    }
  }

  if (!stop_it) {
    m_SiteList.push_back(site);
    //LOG(INFO) << "Link " << site->name() << " with cycle " << m_CycleName;
    rc = OK;
  }
  else {
  }

  return rc;
}

#if 0
// ===================================================================================================
CycleList::CycleList()
{
  m_Cycles.clear();
}

// ===================================================================================================
CycleList::~CycleList()
{
  for (std::vector<Cycle*>::iterator it = m_Cycles.begin(); it != m_Cycles.end(); ++it)
  {
#if VERBOSE>7
    LOG(INFO) << "free cycle " << (*it)->name();
#endif
    delete (*it);
  }
}

// ===================================================================================================
size_t CycleList::insert(Cycle* new_cycle)
{
  m_Cycles.push_back(new_cycle);
  return m_Cycles.size();
}

// ===================================================================================================
// Вернуть экземпляр Цикла с указанным числовым идентификатором
Cycle* CycleList::operator[](size_t idx)
{
  Cycle *look = NULL;

  for (std::vector<Cycle*>::const_iterator it = m_Cycles.begin(); it != m_Cycles.end(); ++it)
  {
    if ((*it)->id() == idx) {
      look = (*it);
      break;
    }
  }

  return look;
}

// ===================================================================================================
// Вернуть экземпляр Цикла с указанным названием
Cycle* CycleList::operator[](const std::string& name)
{
  Cycle *look = NULL;

  for (std::vector<Cycle*>::const_iterator it = m_Cycles.begin(); it != m_Cycles.end(); ++it)
  {
    if (0 == name.compare((*it)->name())) {
      look = (*it);
      break;
    }
  }

  return look;
}
#endif
// ===================================================================================================