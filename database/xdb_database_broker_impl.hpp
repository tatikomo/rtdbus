#if !defined XDB_DATABASE_BROKER_IMPL_HPP
#define XDB_DATABASE_BROKER_IMPL_HPP
#pragma once

#include <string>

#ifdef __cplusplus
extern "C" {
#include "mco.h"
}
#endif

#include "xdb_database_broker.hpp"

class Service;
class Worker;

#define DBNAME_MAXLEN 10

/* Фактическая реализация функциональности Базы Данных для Брокера */
class XDBDatabaseBrokerImpl
{
  public:
    XDBDatabaseBrokerImpl(const XDBDatabaseBroker*, const char*);
    ~XDBDatabaseBrokerImpl();
    bool Open();
    bool AddService(const char*);
    bool RemoveService(const char*);
    /* Удалить Обработчик из всех связанных с ним таблиц БД */
    bool RemoveWorker(Worker*);
    bool IsServiceExist(const char*);

    /* Вернуть экземпляр Сервиса. Если он не существует в БД - создать */
    Service *RequireServiceByName(const char*);
    Service *RequireServiceByName(const std::string&);

    /* Вернуть экземпляр Обработчика из БД. Не найден - вернуть NULL */
    Worker *GetWorkerByIdent(const char*);
    Worker *GetWorkerByIdent(const std::string&);

    Service *GetServiceByName(const char*);
    Service *GetServiceByName(const std::string&);
    Service *GetServiceById(int64_t _id);
    Service *GetServiceForWorker(const Worker*);

    Worker *GetWorkerForService(const Service*);
    Worker *PopWorkerForService(const char*);
    Worker *PopWorkerForService(const std::string&);
    Worker *PopWorkerForService(Service*);

    /* Добавить нового Обработчика в спул Сервиса */
    bool PushWorkerForService(Service*, Worker*);
    /* TODO Worker должен содержать сведения о своем Сервисе */
    bool PushWorker(Worker*);

    bool ClearWorkersForService(const char*);
    bool ClearWorkersForService(const std::string&);

    bool ClearServices();

    void EnableServiceCommand (const std::string&, const std::string&);
    void EnableServiceCommand (const Service*, const std::string&);
    void DisableServiceCommand (const std::string&, const std::string&);
    void DisableServiceCommand (const Service*, const std::string&);

  private:
    const XDBDatabaseBroker *m_self;
    mco_db_h                 m_db;
    char                     m_name[DBNAME_MAXLEN+1];
    void  LogError(MCO_RET, const char*, const char*);
    bool  AttachToInstance();
#ifdef DISK_DATABASE
    char* m_dbsFileName;
    char* m_logFileName;
#endif
};

#endif
