#include "config.h"
#include "xdb_rtap_error.hpp"

using namespace xdb;

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

void RtError::init()
{
  m_error_descriptions[rtE_NONE]             = "No error";
  m_error_descriptions[rtE_UNKNOWN]          = "Unknown error";
  m_error_descriptions[rtE_NOT_IMPLEMENTED]  = "Not yet implemented";
  m_error_descriptions[rtE_STRING_TOO_LONG]  = "Given string is too long";
  m_error_descriptions[rtE_STRING_IS_EMPTY]  = "Given string is empty";
}

bool RtError::set(ErrorType_t _t)
{
  bool status = false;

  if ((rtE_NONE < _t) && (_t < rtE_LAST))
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

