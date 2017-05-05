#if defined HAVE_CONFIG_H
#include "config.h"
#endif
#include "xdb_impl_error.hpp"

using namespace xdb;

const char* g_error_descriptions[Error::MaxErrorCode + 1];

static __attribute__((constructor)) void my_errortable_init()
{
  g_error_descriptions[rtE_NONE]             = (const char*)"OK";
  g_error_descriptions[rtE_UNKNOWN]          = (const char*)"Unknown error";
  g_error_descriptions[rtE_NOT_IMPLEMENTED]  = (const char*)"Not yet implemented";
  g_error_descriptions[rtE_STRING_TOO_LONG]  = (const char*)"Given string is too long";
  g_error_descriptions[rtE_STRING_IS_EMPTY]  = (const char*)"Given string is empty";
  g_error_descriptions[rtE_DB_NOT_FOUND]     = (const char*)"Database is not found";
  g_error_descriptions[rtE_DB_NOT_OPENED]    = (const char*)"Database is not opened";
  g_error_descriptions[rtE_DB_NOT_DISCONNECTED] = (const char*)"Database is not disconnected";
  g_error_descriptions[rtE_XML_OPEN]            = (const char*)"Error opening XML snap";
  g_error_descriptions[rtE_INCORRECT_DB_STATE]  = (const char*)"New DB state is incorrect";
  g_error_descriptions[rtE_SNAPSHOT_WRITE]   = (const char*)"Snapshot writing failure";
  g_error_descriptions[rtE_SNAPSHOT_READ]    = (const char*)"Snapshot reading failure";
  g_error_descriptions[rtE_SNAPSHOT_NOT_EXIST]  = (const char*)"Snapshot not exist";
  g_error_descriptions[rtE_RUNTIME_FATAL]    = (const char*)"Fatal runtime failure";
  g_error_descriptions[rtE_RUNTIME_ERROR]    = (const char*)"Runtime error";
  g_error_descriptions[rtE_RUNTIME_WARNING]  = (const char*)"Recoverable runtime failure";
  g_error_descriptions[rtE_TABLE_CREATE]     = (const char*)"Creating table";
  g_error_descriptions[rtE_TABLE_READ]       = (const char*)"Reading from table";
  g_error_descriptions[rtE_TABLE_WRITE]      = (const char*)"Writing into table";
  g_error_descriptions[rtE_TABLE_DELETE]     = (const char*)"Deleting table";
  g_error_descriptions[rtE_POINT_CREATE]     = (const char*)"Creating point";
  g_error_descriptions[rtE_POINT_READ]       = (const char*)"Reading point";
  g_error_descriptions[rtE_POINT_WRITE]      = (const char*)"Writing point";
  g_error_descriptions[rtE_POINT_DELETE]     = (const char*)"Deleting point";
  g_error_descriptions[rtE_ILLEGAL_PARAMETER_VALUE] = (const char*)"Illegal parameter value";
  g_error_descriptions[rtE_POINT_NOT_FOUND]  = (const char*)"Specific point is not found";
  g_error_descriptions[rtE_ATTR_NOT_FOUND]   = (const char*)"Specific attribute is not found";
  g_error_descriptions[rtE_ILLEGAL_TAG_NAME] = (const char*)"Illegal point's tag";
  g_error_descriptions[rtE_ALREADY_EXIST]    = (const char*)"Object already exist";
  g_error_descriptions[rtE_CONNECTION_INVALID] = (const char*)"Connection not valid";
  g_error_descriptions[rtE_DATABASE_INSTANCE_DUPLICATE] = (const char*)"Database instance duplicate";
}

Error::Error() :
  m_error_code(rtE_NONE)
{
}

Error::Error(ErrorCode_t _t) :
  m_error_code(_t)
{
}

/*Error::Error(int _t)
{
  init();
  set(_t);
}*/

Error::Error(const Error& _origin)
 : m_error_code(static_cast<ErrorCode_t>(_origin.getCode()))
{
}

Error::~Error()
{
}

const char* Error::what() const
{
  return g_error_descriptions[m_error_code];
}

const char* Error::what(int code)
{
  const char* message = 0;

  if ((rtE_NONE <= code) && (code <= rtE_LAST))
  {
    message = g_error_descriptions[code];
  }
  return message;
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

