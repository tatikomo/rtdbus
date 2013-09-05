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
class ServiceListImpl;

/* 
 * Класс-контейнер объектов Service в БД
 * TODO: Получать уведомления о создании/удалении экземпляра объекта Service в БД.
 * TODO: Переделать с использованием итераторов.
 */
class ServiceListImpl : public  ServiceList
{
  public:
    ServiceListImpl(mco_db_h);
    ~ServiceListImpl();

    Service* first();
    Service* last();
    Service* next();
    Service* prev();

    // Добавить новый Сервис, определенный своим именем и идентификатором
    bool AddService(const char*, int64_t);
    // Добавить новый Сервис, определенный объектом Service
    bool AddService(const Service*);
    // Удалить Сервис по его имени
    bool RemoveService(const char*);
    // Удалить Сервис по его идентификатору
    bool RemoveService(const int64_t);

    // Получить количество зарегистрированных объектов
    const int size();
    // Перечитать список Сервисов из базы данных
    bool refresh();

  private:
    DISALLOW_COPY_AND_ASSIGN(ServiceListImpl);
    int       m_current_index;
    mco_db_h  m_db;
    // Список прочитанных из БД Сервисов
    Service** m_array;
    // Количество Сервисов в массиве m_array
    int       m_size;
};

/* Фактическая реализация функциональности Базы Данных для Брокера */
class XDBDatabaseBrokerImpl
{
  friend class ServiceListImpl;
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
    /* Установить новое состояние Обработчику */
    bool SetWorkerState(Worker*, Worker::State);
    /* получить признак существования данного экземпляра Сервиса в БД */
    bool     IsServiceExist(const char*);
    /* получить доступ к текущему списку Сервисов */ 
    ServiceList* GetServiceList();
    /* Получить первое ожидающее обработки Сообщение */
    bool GetWaitingLetter(/* IN */ Service* srv,
        /* IN */  Worker* wrk,
        /* OUT */ std::string& header,
        /* OUT */ std::string& body);


    bool IsServiceCommandEnabled(const Service*, const std::string&);

    /* поместить сообщение во входящую очередь Службы */
    bool PushRequestToService(Service*, const std::string&, const std::string&);

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

//    Worker *GetWorker(const Service*);
//    Worker *PopWorker(const char*);
    Worker *PopWorker(const std::string&);
    Worker *PopWorker(const Service*);

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
    DISALLOW_COPY_AND_ASSIGN(XDBDatabaseBrokerImpl);
#if defined DEBUG
    char  m_snapshot_file_prefix[10];
    bool  m_initialized;
    bool  m_save_to_xml_feature;
    int   m_snapshot_counter;
    MCO_RET SaveDbToFile(const char*);
#endif

    XDBDatabaseBroker       *m_self;
    mco_db_h                 m_db;
    ServiceListImpl         *m_service_list;

#if EXTREMEDB_VERSION >= 41
    mco_db_params_t   m_db_params;
    mco_device_t      m_dev;
#  if USE_EXTREMEDB_HTTP_SERVER  
    /*
     * Internal HttpServer http://localhost:8082/
     */
    mco_metadict_header_t *m_metadict;
    mcohv_p                m_hv;
    unsigned int           m_size;
#  endif  
    bool                   m_metadict_initialized;
#endif
    /*
     * Зарегистрировать все обработчики событий, заявленные в БД
     */
    MCO_RET RegisterEvents();
    /*
     * Автоматически вызываемые события при работе с экземплярами XDBService
     */
    static MCO_RET new_Service(mco_trans_h, XDBService*, MCO_EVENT_TYPE, void*);
    static MCO_RET del_Service(mco_trans_h, XDBService*, MCO_EVENT_TYPE, void*);
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
     * Вернуть новый Worker, построенный на основе прочитанных из БД данных
     * autoid_t - идентификатор Сервиса, содержащий данный Обработчик
     */
    MCO_RET LoadWorker(mco_trans_h,
                       xdb_broker::XDBWorker&,
                       Worker*&);

    /*
     * Поиск Обработчика, находящегося в заданном состоянии. 
     * Возвращает статус поиска и экземпляр найденного Обработчика
     */
    Worker* GetWorkerByState(autoid_t& service_id,
                       WorkerState);

    /*
     * Прочитать состояние Обработчика по значению его identity
     */
    //MCO_RET LoadWorkerByIdent(mco_trans_h, autoid_t&, Worker*);

#ifdef DISK_DATABASE
    char* m_dbsFileName;
    char* m_logFileName;
#endif
};

#endif
