#include "config.h"
#include "xdb_rtap_attribute.hpp"
#include "xdb_rtap_error.hpp"

using namespace xdb;

RtAttribute::RtAttribute() :
  m_last_error(rtE_NONE)
{
}

RtAttribute::~RtAttribute()
{
}

const char* RtAttribute::getAlias() const
{
  return &m_alias[0];
}

DbType_t RtAttribute::getAttributeType() const
{
  return m_type;
}
