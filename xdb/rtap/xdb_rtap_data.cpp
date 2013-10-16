#include <assert.h>

#include "config.h"
#include "xdb_rtap_error.hpp"
#include "xdb_rtap_data.hpp"

using namespace xdb;


RtData::RtData()
{
  init();
}

RtData::~RtData()
{
}

void RtData::init()
{
    DbTypeDescription[DB_TYPE_UNDEF]       = "undef";
    DbTypeDescription[DB_TYPE_INTEGER8]    = "int8";
    DbTypeDescription[DB_TYPE_INTEGER16]   = "int16";
    DbTypeDescription[DB_TYPE_INTEGER32]   = "int32";
    DbTypeDescription[DB_TYPE_INTEGER64]   = "int64";
    DbTypeDescription[DB_TYPE_FLOAT]       = "float";
    DbTypeDescription[DB_TYPE_DOUBLE]      = "double";
    DbTypeDescription[DB_TYPE_BYTES]       = "bytes";
}

uint8_t  RtData::getInt8Value() const
{
  return m_attr_value.val_int8;
}

uint16_t  RtData::getInt16Value() const
{
  return m_attr_value.val_int16;
}

uint32_t  RtData::getInt32Value() const
{
  return m_attr_value.val_int32;
}

float  RtData::getFloatValue() const
{
  return m_attr_value.val_float;
}

double  RtData::getDoubleValue() const
{
  return m_attr_value.val_double;
}

unsigned char* RtData::getBytesValue() const
{
  return m_attr_value.val_bytes.data;
}

