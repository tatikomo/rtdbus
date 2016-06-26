#include <iostream>

#include <assert.h>
#include <string.h>
#include <algorithm>

#include "glog/logging.h"
#if defined HAVE_CONFIG_H
#include "config.h"
#endif
#include "helper.hpp"
#include "xdb_impl_error.hpp"
#include "xdb_impl_application.hpp"
#include "xdb_impl_environment.hpp"
#include "xdb_rtap_environment.hpp"
#include "xdb_rtap_application.hpp"

using namespace xdb;

// ---------------------------------------------------------------------
RtApplication::RtApplication(const char* _name)
  : m_impl(new ApplicationImpl(_name)),
    m_initialized(false)
{
  // Установить вместимость списка Сред - не более четырёх
  m_env_list.reserve(4);
}

// ---------------------------------------------------------------------
RtApplication::~RtApplication()
{
  for (size_t env_nbr = 0; env_nbr < m_env_list.size(); env_nbr++)
  {
    LOG(INFO) << "Delete environment " << m_env_list[env_nbr]->getName();
    delete m_env_list[env_nbr];
  }
  m_env_list.clear();

  delete m_impl;
}

// ---------------------------------------------------------------------
// TODO: провести инициализацию рантайма с учетом данных начальных условий
const Error& RtApplication::initialize()
{
  if (m_initialized)
  {
    LOG(WARNING) << "Try to multiple initialize RtApplication " << m_impl->getAppName();
    m_impl->setLastError(rtE_RUNTIME_WARNING);
  }
  else
  {
    LOG(INFO) << "RtApplication::initialize()";
    m_impl->initialize();
    m_initialized = true;
  }
  return m_impl->getLastError();
}

// ---------------------------------------------------------------------
ApplicationImpl* RtApplication::getImpl()
{
  return m_impl;
}

// ---------------------------------------------------------------------
const ::Options* RtApplication::getOptions() const
{
  return m_impl->getOptions();
}

// ---------------------------------------------------------------------
bool RtApplication::getOption(const std::string& key, int& val)
{
  return m_impl->getOption(key, val);
}

// ---------------------------------------------------------------------
void RtApplication::setOption(const char* key, int val)
{
  m_impl->setOption(key, val);
}

// ---------------------------------------------------------------------
const char* RtApplication::getAppName() const
{
  return m_impl->getAppName();
}

// ---------------------------------------------------------------------
const Error& RtApplication::getLastError() const
{
  return m_impl->getLastError();
}

// ---------------------------------------------------------------------
AppMode_t RtApplication::getOperationMode() const
{
  return m_impl->getOperationMode();
}

// ---------------------------------------------------------------------
AppState_t RtApplication::getOperationState() const
{
  return m_impl->getOperationState();
}

// ---------------------------------------------------------------------
// Загрузить среду с заданным именем, или среду по умолчанию
// Фактическое содание БД произойдет в RtEnvironment.Start()
// TODO: Найти файл с описателем данной среды (аналог RtapEnvTable) и НСИ
RtEnvironment* RtApplication::loadEnvironment(const char* _env_name)
{
  Error status;
  int   val;
  RtEnvironment *env = isEnvironmentRegistered(_env_name);
  
  if (env)
  {
    LOG(INFO) << "Application '" << m_impl->getAppName()
              << "' is loading environment '" << _env_name << "'";
    return env;
  }

  // Создать экземпляр RtEnvironment
  env = getEnvironment(_env_name);

  // Загрузить ранее сохраненное содержимое только при включенной LOAD_SNAP
  if (getOption("OF_LOAD_SNAP", val) && val)
  {
    status = env->LoadSnapshot(); // Имя по-умолчанию: НАЗВАНИЕ_СРЕДЫ.snap.xml

    switch(status.code())
    {
      case rtE_NONE:
        LOG(INFO) << "'" << _env_name << "' DB content is loaded succesfully";
        break;
      
      case rtE_SNAPSHOT_NOT_EXIST:
        // TODO восстановить состояние по конфигурационным файлам
        LOG(ERROR) << "Construct empty database contents for '"
                   << m_impl->getAppName() << ":" << env->getName() << "'";
        status.set(rtE_NOT_IMPLEMENTED);
        break;

      case rtE_DATABASE_INSTANCE_DUPLICATE:
        // Возможно, это повторный запуск после аварийного сбоя.
        // БД уже загружена в разделяемую память, просто подключиться к ней.
        delete env;
        env = attach_to_env(_env_name);
        env->clearError();
        status.clear();
        break;

      case rtE_SNAPSHOT_READ:
        // TODO восстановить состояние по конфигурационным файлам
        LOG(ERROR) << "TODO: Recovery database contents for '"
                   << m_impl->getAppName() << ":" << env->getName() << "'";
        status.set(rtE_NOT_IMPLEMENTED);
        break;

      default:
        LOG(ERROR) << m_impl->getAppName() << " fault loads '" << env->getName()
                   << "' DB content from its snapshot";
    }

    if (status.Ok())
    {
      // Владение экземпляром перешло к RtApplication
      registerEnvironment(env);
    }
    else
    {
      // Удаляем экземпляр Среды, созданный с ошибкой
      delete env;
      env = NULL;
    }
  }
  else
  {
    LOG(INFO) << "Using clean content '"<< env->getName() <<"' database";

    // Владение экземпляром перешло к RtApplication
    registerEnvironment(env);

    LOG(INFO) << "Environment '" << env->getName()
              << "' is loaded in App '" << m_impl->getAppName() << "'";
  }

  return env;
}

// ---------------------------------------------------------------------
// TODO: БДРВ может быть создана другим процессом, и находиться в общем
// сегменте разделяемой памяти. В таком случае необходимо найти такую БДРВ
// среди других возможно существующих экземпляров XDB.
//
RtEnvironment* RtApplication::attach_to_env(const char* _env_name)
{
  // Попытаемся определить, был ли уже текущий процесс подключен к этой БД?
  // Если env равно NULL - не был подключен.
  RtEnvironment *env = isEnvironmentRegistered(_env_name);
  
  if (env)
  {
    LOG(INFO) << "Application '" << m_impl->getAppName()
              << "' is attached to environment '" << _env_name;
  }
  else
  {
    if (true == m_impl->database_instance_presence(_env_name))
    {
      // Экземпляр БД есть в памяти, и текущий процесс не был подключен - подключиться
      // создать и зарегистрировать экземпляр RtEnvironment
      env = new RtEnvironment(this, _env_name);
      LOG(INFO) << "Attach to external instance '" << _env_name << "'";
      // Владение экземпляром перешло к RtApplication
      registerEnvironment(env);
    }
  }

  return env;
}

// ---------------------------------------------------------------------
// Зарегистрировать в Приложении новую Среду, кроме дубликатов 
void RtApplication::registerEnvironment(RtEnvironment* _new_env)
{
  if (!isEnvironmentRegistered(_new_env->getName()))
  {
    if (m_env_list.size() <= 4) {
      LOG(INFO) << "Register environment " << _new_env->getName();
      m_env_list.push_back(_new_env);
    }
    else {
      LOG(ERROR) << "Exceed environments list capacity (4) while insert " << _new_env->getName();
    }
  }
  else
  {
    LOG(WARNING) << "Try to register already registered environment " << _new_env->getName();
  }
}

// ---------------------------------------------------------------------
// Вернуть ссылку на объект-среду по её имени
// NB: Если имя не задано, вернуть среду по-умолчанию
RtEnvironment* RtApplication::getEnvironment(const char* _env_name)
{
  const char *name = (_env_name)? _env_name : m_impl->getAppName();

  RtEnvironment *env = isEnvironmentRegistered(name);

  if (!env)
  {
    env = new RtEnvironment(this, name);
    LOG(INFO) << "Creating new environment '" << m_impl->getAppName()
              << ":" << name << "'";
  }
  return env;
}

// ---------------------------------------------------------------------
RtEnvironment*  RtApplication::isEnvironmentRegistered(const char* _env_name)
{
  RtEnvironment *env = NULL;
  const char *name = (_env_name)? _env_name : m_impl->getAppName();

  LOG(INFO) << "Search environment " << name;

  //  foreach();
  //  TODO: найти подобный объект по имени в спуле m_env_list.
  //  Если найти не удалось, создать новый и поместить в спул.
  for (size_t env_nbr = 0; env_nbr < m_env_list.size(); env_nbr++)
  {
    env = m_env_list[env_nbr];
    if (0 == strcmp(env->getName(), name))
    {
      LOG(INFO) << "We found " << name;
      break;
    }
    // При выходе из цикла в случае неуспеха поиска env д.б. равен 0
    env = NULL;
  }

  return env;
}
// ---------------------------------------------------------------------

