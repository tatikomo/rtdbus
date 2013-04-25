#if !defined XDB_DATABASE_WORKER_HPP
#define XDB_DATABASE_WORKER_HPP
#pragma once

#include <stdint.h>

/*
 * Содержит
 * 1. Название Обработчика
 * 2. Строковый идентификатор
 * 3. Ссылку на Сервис 
 */
class XDBWorker
{
  public:
    XDBWorker();
    /* 
     * NB: размер поля данных идентификатора 
     * задан в структуре point_oid файла broker.mco
     */
    XDBWorker(const int64_t, const char*);
    XDBWorker(const char*);
    ~XDBWorker();
    void SetIDENTITY(const char*);
    void SetID(int64_t);
    const int64_t GetID();
    const char* GetIDENTITY();

  private:
    int64_t  m_id;
    char    *m_identity;
    bool     m_modified;
};

#endif

