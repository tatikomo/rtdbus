#if !defined XDB_DATABASE_SERVICE_HPP
#define XDB_DATABASE_SERVICE_HPP
#pragma once

#include <stdint.h>

/*
 * Содержит
 * 1. Название сервиса
 * 1. список Обработчиков (Worker-ов)
 * 2. список Команд 
 */
class Service
{
  public:
    Service();
    /* 
     * NB: размер поля данных идентификатора 
     * задан в структуре point_oid файла broker.mco
     */
    Service(const int64_t, const char*);
    ~Service();
    void SetID(const int64_t);
    void SetNAME(const char*);
    void SetWaitingWorkersCount(int);
    const int64_t GetID();
    const char   *GetNAME();
    const char   *GetSERVICE_NAME();
    const int     GetWaitingWorkersCount();


  private:
    int64_t  m_id;
    char    *m_name;
    char    *m_service_name;
    bool     m_modified;
    int      m_waiting_workers_count;
};

#endif

