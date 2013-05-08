#if !defined XDB_DATABASE_WORKER_HPP
#define XDB_DATABASE_WORKER_HPP
#pragma once

#include <stdint.h>

#define WORKER_IDENTITY_MAXLEN 11

/*
 * Содержит
 * 1. Название Обработчика
 * 2. Строковый идентификатор
 * 3. Ссылку на Сервис 
 */
class Worker
{
  public:
    Worker();
    /* 
     * NB: размер поля данных идентификатора 
     * задан в структуре point_oid файла broker.mco
     */
    Worker(const int64_t self_id, const char *identity, const int64_t service_id);
    ~Worker();
    void SetIDENTITY(const char*);
    void SetID(int64_t);
    void SetEXPIRATION(int64_t);
    const int64_t GetID();
    const int64_t GetEXPIRATION();
    const int64_t GetSERVICE_ID();
    const char* GetIDENTITY();
    bool Expired();

  private:
    int64_t  m_id;
    int64_t  m_expiration;
    int64_t  m_service_id;
    char    *m_identity;
    bool     m_modified;

    void     SetSERVICE_ID(int64_t);
};

#endif

