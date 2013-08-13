#if !defined XDB_DATABASE_BROKER_IMPL_HPP
#define XDB_DATABASE_BROKER_IMPL_HPP
#pragma once

#include <string>
#include "config.h"

#ifdef __cplusplus
extern "C" {
#include "mco.h"
#if EXTREMEDB_VERSION >= 41
#include "mcohv.h"
#endif
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
    XDBDatabaseBrokerImpl(XDBDatabaseBroker*);
    ~XDBDatabaseBrokerImpl();

    /* Инициализация служебных структур БД */
    bool Init();
    /* Создание экземпляра БД или подключение к уже существующему */
    bool Connect();
    bool Disconnect();

    Service *AddService(const char*);
    Service *AddService(const std::string&);
    bool RemoveService(const char*);
    /* Удалить Обработчик из всех связанных с ним таблиц БД */
    bool RemoveWorker(Worker*);
    /* Добавить нового Обработчика в спул Сервиса */
    bool PushWorkerForService(const Service*, Worker*);// TODO: может быть удалить?
    Worker* PushWorkerForService(const std::string&, const std::string&);
    /* поместить Обработчик в спул своего Сервиса */
    bool PushWorker(Worker*);
    /* получить признак существования данного экземпляра Сервиса в БД */
    bool     IsServiceExist(const char*);

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

    XDBDatabaseBroker       *m_self;
    mco_db_h                 m_db;

#if EXTREMEDB_VERSION >= 41
    mco_db_params_t   m_db_params;
    mco_device_t      m_dev;
#  if USE_EXTREMEDB_HTTP_SERVER  
    /*
     * HttpServer
     */
    mco_metadict_header_t *m_metadict;
    mcohv_p                m_hv;
    unsigned int           m_size;
#  endif  
    bool                   m_metadict_initialized;
#endif
    /*
     * Подключиться к БД, а при ее отсутствии - создать
     */
    bool  AttachToInstance();

    /* 
     * Вернуть Service, построенный на основе прочитанных из БД данных
     */
    Service* LoadService(autoid_t&,
                         xdb_broker::XDBService&);

    /* 
     * Вернуть Worker, построенный на основе прочитанных из БД данных
     * autoid_t - идентификатор Сервиса, содержащий данный Обработчик
     */
    Worker* LoadWorker(mco_trans_h,
                       autoid_t&,
                       xdb_broker::XDBWorker&,
                       uint16_t);

    /*
     * Поиск в спуле данного Сервиса индекса Обработчика,
     * находящегося в состоянии state. 
     * Возвращает индекс найденного Обработчика, или -1
     */
    uint2 LocatingFirstOccurence(xdb_broker::XDBService &service_instance,
                       WorkerState            state);

    /*
     * Прочитать состояние Обработчика по значению его identity
     */
    MCO_RET LoadWorkerByIdent(mco_trans_h, Service*, Worker*);

#ifdef DISK_DATABASE
    char* m_dbsFileName;
    char* m_logFileName;
#endif
};

#endif
