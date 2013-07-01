#if !defined XDB_DATABASE_BROKER_IMPL_HPP
#define XDB_DATABASE_BROKER_IMPL_HPP
#pragma once

#include <string>
#include "config.h"

#ifdef __cplusplus
extern "C" {
#include "mco.h"
}
#endif

#include "dat/xdb_broker.hpp"
#include "xdb_database_broker.hpp"
#include "xdb_database_service.hpp"

class Service;
class Worker;

/* Фактическая реализация функциональности Базы Данных для Брокера */
class XDBDatabaseBrokerImpl
{
  public:
    enum State {
      SHUTDOWN, INIT
    };

    XDBDatabaseBrokerImpl(const XDBDatabaseBroker*, const char*);
    ~XDBDatabaseBrokerImpl();

    bool Open();
    bool AddService(const char*);
    bool RemoveService(const char*);
    /* Удалить Обработчик из всех связанных с ним таблиц БД */
    bool RemoveWorker(Worker*);
    /* Добавить нового Обработчика в спул Сервиса */
    bool PushWorkerForService(Service*, Worker*);
    /* поместить Обработчик в спул своего Сервиса */
    bool PushWorker(Worker*);
    bool IsServiceExist(const char*);
    bool IsServiceCommandEnabled(const Service*, const std::string&);

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
    // Вернуть текущее состояние Сервиса
    Service::State GetServiceState(const Service*);

    Worker *GetWorker(const Service*);
    Worker *PopWorker(const char*);
    Worker *PopWorker(const std::string&);
    Worker *PopWorker(Service*);

    bool ClearWorkersForService(const char*);
    bool ClearWorkersForService(const std::string&);

    bool ClearServices();

    void EnableServiceCommand (const std::string&, const std::string&);
    void EnableServiceCommand (const Service*, const std::string&);
    void DisableServiceCommand (const std::string&, const std::string&);
    void DisableServiceCommand (const Service*, const std::string&);

    /* Тестовый API сохранения базы */
    void MakeSnapshot(const char*);

  private:
#if defined DEBUG
    char  m_snapshot_file_prefix[10];
    bool  m_initialized;
    bool  m_save_to_xml_feature;
    int   m_snapshot_counter;
    MCO_RET SaveDbToFile(const char*);
#endif

    const XDBDatabaseBroker *m_self;
    mco_db_h                 m_db;
    char                     m_name[DBNAME_MAXLEN+1];
    State                    m_state;
    void  LogError(MCO_RET, const char*, const char*);
    void  LogWarn(const char*, const char*);
    bool  AttachToInstance();

    /* 
     * Вернуть Service, построенный на основе прочитанных из БД данных
     */
    Service* LoadService(mco_trans_h,
                        autoid_t&,
                        xdb_broker::XDBService&);

    /* 
     * Вернуть Worker, построенный на основе прочитанных из БД данных
     * autoid_t - идентификатор Сервиса, содержащий данный Обработчик
     */
    Worker* LoadWorker(mco_trans_h,
                       autoid_t&,
                       xdb_broker::XDBWorker&);

    /*
     * Поиск в спуле данного Сервиса индекса Обработчика,
     * находящегося в состоянии state. 
     * Возвращает индекс найденного Обработчика, или -1
     */
    uint2 LocatingFirstOccurence(xdb_broker::XDBService &service_instance,
                       WorkerState            state);

#ifdef DISK_DATABASE
    char* m_dbsFileName;
    char* m_logFileName;
#endif
};

#endif
