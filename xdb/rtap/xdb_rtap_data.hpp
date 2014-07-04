#if !defined XDB_RTAP_DATA_H_
#define XDB_RTAP_DATA_H_
#pragma once

#include <string>
#include "config.h"
#include "xdb_core_error.hpp"
#include "xdb_rtap_const.hpp"

namespace xdb {
namespace rtap {

class RtData
{
   public:
     RtData();
     ~RtData();

     // Returns a string describing the given data element type
     const char* deTypeToString(xdb::core::DbType_t);
     //
     uint8_t  getInt8Value() const;
     //
     uint16_t getInt16Value() const;
     //
     uint32_t getInt32Value() const;
     //
     float    getFloatValue() const;
     //
     double   getDoubleValue() const;
     // Предоставить доступ к внутреннему значению как к массиву байт
     char* getBytesValue() const;
     //
     xdb::core::DbType_t getDeType() const;
     //
     void setValue(uint8_t);
     void setValue(int8_t);
     void setValue(uint16_t);
     void setValue(int16_t);
     void setValue(uint32_t);
     void setValue(int32_t);
     void setValue(float);
     void setValue(double);
     void setValue(char*);
     void setValue(xdb::core::variable_t);

     const xdb::core::Error& getLastError() const { return m_last_error; }

   private:
     DISALLOW_COPY_AND_ASSIGN(RtData);
     void       init();
     xdb::core::Error      m_last_error;
     xdb::core::DbType_t   m_attr_type;
     xdb::core::AttrVal_t  m_attr_value;
     bool       m_modified;
};

} // namespace rtap
} // namespace xdb

#endif

