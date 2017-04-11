#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

// Общесистемные заголовочные файлы
#include <assert.h>
#include <unistd.h>
#include <linux/limits.h>

// Служебные заголовочные файлы сторонних утилит
#include "glog/logging.h"

// Служебные файлы RTDBUS
#include "exchange_egsa_cycle.hpp"
#include "tool_timer.hpp"

// ===================================================================================================
class CycleTrigger : public TimerTimeoutHandler
{
  public:
    CycleTrigger(const std::string&);
    CycleTrigger(const std::string&, int);
   ~CycleTrigger() {};

    void handlerFunction( void );

  private:
    std::string m_cycle_name;
    int m_fd;
};

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

// ===================================================================================================
Cycle::~Cycle()
{
//  delete m_CycleTimer;
//  delete m_CycleTrigger;
};

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
#endif

// ===================================================================================================
// Проверить, зарегистрирована ли указанная СС в данном Цикле
bool Cycle::exist_for_SA(const std::string& sa_name)
{
  bool found = false;

  // Ищем, если есть список сайтов не пуст
  if (m_AcqSites.size() > 0) {
    // Проверить все связанные СС
    for (std::vector<acq_site_state_t>::const_iterator it = m_AcqSites.begin();
         it != m_AcqSites.end();
         ++it) {
      // Если найдена СС, имеющая этот цикл
      if (0 == sa_name.compare((*it).site)) {
        LOG(INFO) << "found cycle: " << m_CycleName << " for " << sa_name;
        found = true;
        break;
      }
    }
  }

  return found;
}

// ===================================================================================================
// Зарегистрировать указанную СС в этом Цикле
int Cycle::register_SA(const std::string& sa_name)
{
  acq_site_state_t info;
  int rc = NOK;

  if (!exist_for_SA(sa_name)) {

    strncpy(info.site, sa_name.c_str(), TAG_NAME_MAXLEN);
    info.HCpuLoadReqState = 0; // GEV: определить значение по умолчанию!
    m_AcqSites.push_back(info);
    LOG(INFO) << "Link SA " << sa_name << " with cycle " << m_CycleName;
    rc = OK;
  }
  else {
    LOG(WARNING) << "Try to register in " << m_CycleName << " already known SA: " << sa_name;
  }

  return rc;
}

// ===================================================================================================
