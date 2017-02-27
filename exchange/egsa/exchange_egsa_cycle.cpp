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
    std::string m_message;
    int m_fd;
};

// ===================================================================================================
CycleTrigger::CycleTrigger(const std::string& cycle_name)
  : m_message(cycle_name)
{
//  LOG(INFO) << "New trigger for " << m_message << ", no descriptor";
}

// ===================================================================================================
CycleTrigger::CycleTrigger(const std::string& cycle_name, int fd)
  : m_message(cycle_name),
    m_fd(fd)
{
//  LOG(INFO) << "New trigger for " << m_message << ", fd=" << (int)m_fd;
}

// ===================================================================================================
void CycleTrigger::handlerFunction( void )
{
  ssize_t ssz;

  LOG(INFO) << "time handler invoked: " << m_message << " to fd=" << (int)m_fd;

  if (m_message.size() >= PIPE_BUF) {
    LOG(WARNING) << "Message size (" << m_message.size() << ") is exceed atomic buffer size:" << PIPE_BUF;
  }

  ssz = write(m_fd, m_message.c_str(), m_message.size());
  if (ssz != m_message.size()) {
    LOG(ERROR) << "write less (" << ssz << ") than " << m_message.size() << " bytes to fd="
               << m_fd <<": " << strerror(errno);
  }
}

// ===================================================================================================
Cycle::~Cycle()
{
  delete m_CycleTimer;
  delete m_CycleTrigger;
};

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
  std::vector<acq_site_state_t>::const_iterator it = m_AcqSites.begin();

  // Проверить все связанные СС
  while((it != m_AcqSites.end()) || (!found))
  {
    if (0 == sa_name.compare((*it).site)) {
      LOG(INFO) << "found cycle: " << m_CycleName << "for " << sa_name;

    }
    ++it;
  }
}

// ===================================================================================================
// ===================================================================================================
