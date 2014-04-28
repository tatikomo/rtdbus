#include "config.h"
#include "xdb_rtap_error.hpp"
#include "dat/rtap_db-pimpl.hxx"
#include "dat/rtap_db-pskel.hxx"

using namespace xdb;

char* RtError::m_error_descriptions[RtError::MaxErrorCode + 1];
bool RtError::m_initialized = false;

RtError::RtError() :
  m_error_type(rtE_NONE)
{
  init();
}

RtError::RtError(ErrorType_t _t) :
  m_error_type(_t)
{
  init();
}

RtError::RtError(const RtError& _origin)
{
  m_error_type = _origin.getCode();
  init();
}

RtError::~RtError()
{
}

void RtError::init()
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
    m_error_descriptions[rtE_XML_NOT_OPENED]   = (char*)"XML couldn't be opened";
  }
}

const char* RtError::what() const
{
  return m_error_descriptions[m_error_type];
}

bool RtError::set(ErrorType_t _t)
{
  bool status = false;

  if ((rtE_NONE <= _t) && (_t <= rtE_LAST))
  {
    m_error_type = _t;
    status = true;
  }
  else
  {
    m_error_type = rtE_UNKNOWN;
  }

  return status;
}

// Получить код ошибки
ErrorType_t RtError::getCode() const
{
  return m_error_type;
}

