#include "config.h"
#include "xdb_core_attribute.hpp"
#include "xdb_core_error.hpp"

using namespace xdb;

Attribute::Attribute() :
  m_last_error(rtE_NONE)
{
}

Attribute::~Attribute()
{
}

const char* Attribute::getAlias() const
{
  return &m_alias[0];
}

DbType_t Attribute::getAttributeType() const
{
  return m_type;
}
