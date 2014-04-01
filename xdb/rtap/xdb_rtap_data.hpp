#if !defined XDB_RTAP_DATA_H_
#define XDB_RTAP_DATA_H_
#pragma once

#include <string>
#include "config.h"
#include "xdb_rtap_const.hpp"
#include "xdb_rtap_error.hpp"

namespace xdb
{

class RtData
{
   public:
     RtData();
     ~RtData();

     // Returns a string describing the given data element type
     const char* deTypeToString(DbType_t);
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
     DbType_t getDeType() const;
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
     void setValue(variable_t);

     const RtError& getLastError() const { return m_last_error; }

   private:
     DISALLOW_COPY_AND_ASSIGN(RtData);
     RtError    m_last_error;
     void       init();
     DbType_t   m_attr_type;
     AttrVal_t  m_attr_value;
     bool       m_modified;
};

}
#endif

