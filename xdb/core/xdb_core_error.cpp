#include "config.h"
#include "xdb_core_error.hpp"

using namespace xdb::core;

char* Error::m_error_descriptions[Error::MaxErrorCode + 1];
bool Error::m_initialized = false;

Error::Error() :
  m_error_type(rtE_NONE)
{
  init();
}

Error::Error(ErrorType_t _t) :
  m_error_type(_t)
{
  init();
}

/*Error::Error(int _t)
{
  init();
  set(_t);
}*/

Error::Error(const Error& _origin)
{
  m_error_type = _origin.getCode();
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
    m_error_descriptions[rtE_NONE]             = (char*)"No error";
    m_error_descriptions[rtE_UNKNOWN]          = (char*)"Unknown error";
    m_error_descriptions[rtE_NOT_IMPLEMENTED]  = (char*)"Not yet implemented";
    m_error_descriptions[rtE_STRING_TOO_LONG]  = (char*)"Given string is too long";
    m_error_descriptions[rtE_STRING_IS_EMPTY]  = (char*)"Given string is empty";
    m_error_descriptions[rtE_DB_NOT_FOUND]     = (char*)"Database is not found";
    m_error_descriptions[rtE_DB_NOT_OPENED]    = (char*)"Database is not opened";
    m_error_descriptions[rtE_DB_NOT_DISCONNECTED] = (char*)"Database is not disconnected";
    m_error_descriptions[rtE_XML_NOT_OPENED]   = (char*)"XML couldn't be opened";
    m_error_descriptions[rtE_INCORRECT_DB_TRANSITION_STATE] = (char*)"New DB state is incorrect";
    m_error_descriptions[rtE_SNAPSHOT_WRITE]   = (char*)"Snapshot writing failure";
    m_error_descriptions[rtE_SNAPSHOT_READ]    = (char*)"Snapshot reading failure";
    m_error_descriptions[rtE_RUNTIME_FATAL]    = (char*)"Fatal runtime failure";
    m_error_descriptions[rtE_RUNTIME_WARNING]  = (char*)"Recoverable runtime failure";
  }
}

const char* Error::what() const
{
  return m_error_descriptions[m_error_type];
}

void Error::set(ErrorType_t _t)
{
  if ((rtE_NONE <= _t) && (_t <= rtE_LAST))
  {
    m_error_type = _t;
  }
  else
  {
    m_error_type = rtE_UNKNOWN;
  }
}

// Получить код ошибки
int Error::getCode() const
{
  return m_error_type;
}

