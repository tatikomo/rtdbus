#include <iostream>

#include <assert.h>
#include <string.h>
#include <algorithm>

#include "glog/logging.h"

#if defined HAVE_CONFIG_H
#include "config.h"
#endif

#include "mco.h"

#include "xdb_impl_error.hpp"
#include "xdb_impl_application.hpp"

using namespace xdb;

ApplicationImpl::ApplicationImpl(const char* _name) :
  m_mode(APP_MODE_UNKNOWN),
  m_state(APP_STATE_UNKNOWN),
  m_last_error(rtE_NONE),
  m_map_options()
{
  strncpy(m_appli_name, _name, IDENTITY_MAXLEN);
  m_appli_name[IDENTITY_MAXLEN] = '\0';
}

ApplicationImpl::~ApplicationImpl()
{
}

const char* ApplicationImpl::getAppName() const
{
  return m_appli_name;
}

const Error& ApplicationImpl::setAppName(const char* _name)
{
  m_last_error.clear();
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

// TODO: провести инициализацию рантайма с учетом данных начальных условий
const Error& ApplicationImpl::initialize()
{
  LOG(INFO) << "ApplicationImpl::initialize()";
  m_last_error.clear();

  // Если инициализация не была выполнена ранее или была ошибка
  if (APP_STATE_BAD == getOperationState())
  {
    LOG(ERROR) << "Unable to initialize ApplicationImpl";
  }
  return m_last_error;
}

AppMode_t ApplicationImpl::getOperationMode() const
{
  return m_mode;
}

AppState_t ApplicationImpl::getOperationState() const
{
  return m_state;
}

bool ApplicationImpl::getOption(const std::string& key, int& val)
{
  bool status = ::getOption(&m_map_options, key, val);
  return status;
}

Options* ApplicationImpl::getOptions()
{
  return &m_map_options;
}

void ApplicationImpl::setOption(const char* key, int val)
{
  m_map_options.insert(Pair(std::string(key), val));
  LOG(INFO)<<"Parameter '"<<key<<"': "<<val;
}

const Error& ApplicationImpl::getLastError() const
{
  return m_last_error;
}

void ApplicationImpl::setLastError(const Error& _new_error)
{
  m_last_error.set(_new_error);
}

void ApplicationImpl::clearError()
{
  m_last_error.clear();
}

bool ApplicationImpl::database_instance_presence(const char* env_name)
{
  MCO_RET rc;
  // Размер общего буфера
  const mco_size32_t buffer_size = (DBNAME_MAXLEN+1) * MAX_DATABASE_INSTANCES_PER_SYSTEM;
  // Общий буфер, где лежат названия БД, разделенные символом '\0'
  char lpBuffer[buffer_size + 1];
  // Рабочий указатель, перемещается от имени к имени
  char* ptr = &lpBuffer[0];
  // Номер по порядку экземпляра БД
  int idx = 0;
  // Признак, найдена ли требуемая БД
  bool is_found = false;
  // Количество БД, которые следует пропустить с начала общего буфера
  mco_counter32_t skip_first = 0;

  do
  {
    // Получить названия всех активных БД в памяти
    rc = mco_db_databases(lpBuffer, buffer_size, skip_first);
    if (rc) { LOG(ERROR) << "Get names of all active databases, rc=" << rc; break; }

    while(*ptr)
    {
      LOG(INFO) << "Found active database instance " << (unsigned int) ++idx
                << ", name '" << ptr << "'";
      if (0 == strncmp(env_name, ptr, DBNAME_MAXLEN))
      {
        is_found = true;
      }
      ptr = ptr + strlen(ptr) + 1;
    }

  } while(false);

  return is_found;
}

