#if !defined XDB_DATABASE_SERVICE_HPP
#define XDB_DATABASE_SERVICE_HPP
#pragma once

#include <stdint.h>

class XDBService
{
  public:
    XDBService();
    /* 
     * NB: размер поля данных идентификатора 
     * задан в структуре point_oid файла broker.mco
     */
    XDBService(const int64_t, const char*);
    ~XDBService();
    void SetID(const int64_t);
    void SetNAME(const char*);
    const int64_t GetID();
    const char* GetNAME();

  private:
    int64_t m_id;
    char *m_name;
    bool  m_modified;
};

#endif

