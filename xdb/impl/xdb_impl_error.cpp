#if defined HAVE_CONFIG_H
#include "config.h"
#endif
#include "xdb_impl_error.hpp"

using namespace xdb;

char* Error::m_error_descriptions[Error::MaxErrorCode + 1];
bool Error::m_initialized = false;

Error::Error() :
  m_error_code(rtE_NONE)
{
  init();
}

Error::Error(ErrorCode_t _t) :
  m_error_code(_t)
{
  init();
}

/*Error::Error(int _t)
{
  init();
  set(_t);
}*/

Error::Error(const Error& _origin) :
  m_error_code(_origin.getCode())
{
  init();
}

Error::~Error()
{
}

void Error::init()
{
  if (!m_initialized)
  {
    m_initialized = true;
    m_error_descriptions[rtE_NONE]             = (const char*)"No error";
    m_error_descriptions[rtE_UNKNOWN]          = (const char*)"Unknown error";
    m_error_descriptions[rtE_NOT_IMPLEMENTED]  = (const char*)"Not yet implemented";
    m_error_descriptions[rtE_STRING_TOO_LONG]  = (const char*)"Given string is too long";
    m_error_descriptions[rtE_STRING_IS_EMPTY]  = (const char*)"Given string is empty";
    m_error_descriptions[rtE_DB_NOT_FOUND]     = (const char*)"Database is not found";
    m_error_descriptions[rtE_DB_NOT_OPENED]    = (const char*)"Database is not opened";
    m_error_descriptions[rtE_DB_NOT_DISCONNECTED] = (const char*)"Database is not disconnected";
    m_error_descriptions[rtE_XML_OPEN]         = (const char*)"Error opening XML snap";
    m_error_descriptions[rtE_INCORRECT_DB_TRANSITION_STATE] = (const char*)"New DB state is incorrect";
    m_error_descriptions[rtE_SNAPSHOT_WRITE]   = (const char*)"Snapshot writing failure";
    m_error_descriptions[rtE_SNAPSHOT_READ]    = (const char*)"Snapshot reading failure";
    m_error_descriptions[rtE_RUNTIME_FATAL]    = (const char*)"Fatal runtime failure";
    m_error_descriptions[rtE_RUNTIME_WARNING]  = (const char*)"Recoverable runtime failure";
  }
}

const char* Error::what() const
{
  return m_error_descriptions[m_error_code];
}

void Error::set(ErrorCode_t _t)
{
  if ((rtE_NONE <= _t) && (_t <= rtE_LAST))
  {
    m_error_code = _t;
  }
  else
  {
    m_error_code = rtE_UNKNOWN;
  }
}

// Получить код ошибки
int Error::getCode() const
{
  return static_cast<int>(m_error_code);
}

