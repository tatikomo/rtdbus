#include <assert.h>
#include <string.h>
#include <algorithm>

#include "glog/logging.h"
#include "config.h"

#include "xdb_rtap_error.hpp"
#include "xdb_rtap_application.hpp"
#include "xdb_rtap_environment.hpp"
//#include "xdb_rtap_database.hpp"

using namespace xdb;

RtApplication::RtApplication(const char* _name) :
  m_mode(MODE_UNKNOWN),
  m_state(CONDITION_UNKNOWN),
  m_last_error(rtE_NONE)
{
  m_environment_name[0] = '\0';
  strncpy(m_appli_name, _name, IDENTITY_MAXLEN);
  m_appli_name[IDENTITY_MAXLEN] = '\0';
}

RtApplication::~RtApplication()
{
  m_env_list.clear();
}

const char* RtApplication::getAppName() const
{
  return m_appli_name;
}

const RtError& RtApplication::setAppName(const char* _name)
{
  m_last_error.set(rtE_NONE);
  assert(_name);

  if (_name)
  {
    if (strlen(_name) > IDENTITY_MAXLEN)
    {
      m_last_error.set(rtE_STRING_TOO_LONG);
    }
    else
    {
      strncpy(m_appli_name, _name, IDENTITY_MAXLEN);
      m_appli_name[IDENTITY_MAXLEN] = '\0';
    }
  }
  else
  {
    m_last_error.set(rtE_STRING_IS_EMPTY);
  }

  return m_last_error;
}

const char* RtApplication::getEnvName() const
{
  return m_environment_name;
}

const RtError& RtApplication::setEnvName(const char* _env_name)
{
  m_last_error.set(rtE_NONE);
  strncpy(m_environment_name, _env_name, IDENTITY_MAXLEN);
  return m_last_error;
}

// TODO: провести инициализацию рантайма с учетом данных начальных условий
const RtError& RtApplication::initialize()
{
  m_last_error.set(rtE_NONE);
  m_env_list.clear();
#if 0
  // Если инициализация не была выполнена ранее или была ошибка
  if (CONDITION_BAD == getOperationState())
  {
  }
#endif
  return m_last_error;
}

// Получить ссылку на объект Окружение
RtEnvironment* RtApplication::getEnvironment(const char* _env_name)
{
  RtEnvironment *env = NULL;

  assert(_env_name);
  LOG(INFO) << "Search environment " << _env_name;

  //  foreach();
  //  TODO: найти подобный объект по имени в спуле m_env_list.
  //  Если найти не удалось, создать новый и поместить в спул.
  for (size_t env_nbr = 0; env_nbr < m_env_list.size(); env_nbr++)
  {
    env = m_env_list[env_nbr];
    if (0 == strcmp(env->getName(), _env_name))
    {
      LOG(INFO) << "We found " << _env_name;
      break;
    }
    // При выходе из цикла в случае неуспеха поиска env д.б. равен 0
    env = NULL;
  }

  if (!env)
  {
    env = new RtEnvironment(this, _env_name);
    LOG(INFO) << "We creates environment " << _env_name;
  }
  m_env_list.push_back(env);
  return env;
}

RtApplication::AppMode_t RtApplication::getOperationMode() const
{
  return m_mode;
}

RtApplication::AppState_t RtApplication::getOperationState() const
{
  return m_state;
}

bool RtApplication::getOption(const std::string& key, int& val)
{
  bool status = false;
  OptionIterator p = m_map_options.find(key);

  if (p != m_map_options.end())
  {
    val = p->second;
    status = true;
    std::cout << "Found '"<<key<<"' option=" << p->second << std::endl;
  }
  return status;
}

void RtApplication::setOption(const char* key, int val)
{
  m_map_options.insert(Pair(std::string(key), val));
  LOG(INFO)<<"Parameter '"<<key<<"': "<<val;
}

const RtError& RtApplication::getLastError() const
{
  return m_last_error;
}

