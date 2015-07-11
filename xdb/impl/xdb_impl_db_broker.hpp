#pragma once
#ifndef XDB_DATABASE_BROKER_IMPL_HPP
#define XDB_DATABASE_BROKER_IMPL_HPP

#include <string>
#include "config.h"

// TODO: Удалить реализацию MCO внутрь класса DatabaseImpl
#ifdef __cplusplus
extern "C" {
#include "mco.h"
#if (EXTREMEDB_VERSION >= 50) && USE_EXTREMEDB_HTTP_SERVER
#include "mcohv.h"
#endif
}
#endif

#include "dat/broker_db.hpp"
#include "xdb_broker_letter.hpp"
#include "xdb_broker_worker.hpp"
#include "xdb_broker_service.hpp"
// Прототипы функций типа mco_ret_string, rc_check и show_runtime_info
#include "xdb_common.hpp"

namespace xdb {

class DatabaseImpl;
class Service;
class Worker;

// Структура записи соответствия между названием Сервиса
// и точкой подключения к нему
typedef struct {
  // Название Службы
  const char name[SERVICE_NAME_MAXLEN + 1];
  // Значение по-умолчанию
  const char endpoint_default[ENDPOINT_MAXLEN + 1];
  // Значение, прочитанное из снимка БД
  char endpoint_given[ENDPOINT_MAXLEN + 1];
} ServiceEndpoint_t;

void errhandler(MCO_RET);
void extended_errhandler(MCO_RET errcode, const char* file, int line);

/* 
 * Класс-контейнер объектов Service в БД
 * Вставка/удаление экземпляров осуществляется триггерами (event) БД.
 * TODO: Переделать с использованием итераторов.
 */
class ServiceList
{
  public:
    friend class DatabaseBrokerImpl;

    ServiceList(mco_db_h);
    ~ServiceList();

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
    int size() const;
    // Перечитать список Сервисов из базы данных
    bool refresh();

  private:
    DISALLOW_COPY_AND_ASSIGN(ServiceList);
    int       m_current_index;
    mco_db_h  m_db_handler;
    // Список прочитанных из БД Сервисов
    Service** m_array;
    // Количество Сервисов в массиве m_array
    int       m_size;
};

/* Фактическая реализация функциональности Базы Данных для Брокера */
class DatabaseBrokerImpl
{
  friend class DatabaseImpl;

  public:
    DatabaseBrokerImpl(const char*);
    ~DatabaseBrokerImpl();

    // Создание экземпляра БД или подключение к уже существующему
    bool Connect();
    bool Init();
    bool Disconnect();
    //
    DBState_t State() const;

    // Получить свою точку подключения
    const char* getEndpoint() const;

    // Получить точку подключения для указанного Сервиса
    const char* getEndpoint(const std::string&) const;

    // Создать новый Сервис с указанной точкой подключения
    Service *AddService(const char* /*, const char**/);
    Service *AddService(const std::string& /*, const std::string&*/);

    // получить доступ к текущему списку Сервисов
    ServiceList* GetServiceList();

    // Обновить состояние экземпляра в БД
    bool Update(Worker*);
    bool Update(Service*);

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
    /* Получить первое ожидающее обработки Сообщение */
    // TODO: deprecated
    bool GetWaitingLetter(/* IN */ Service* srv,
        /* IN */  Worker* wrk,
        /* OUT */ std::string& header,
        /* OUT */ std::string& body);
    Letter* GetWaitingLetter(/* IN */ Service*);
    // Найти экземпляр Сообщения по паре Сервис/Обработчик
    Letter* GetAssignedLetter(Worker*);
    // Изменить состояние Сообщения
    bool SetLetterState(Letter*, Letter::State);
    bool AssignLetterToWorker(Worker*, Letter*);
    // Очистить сообщение после получения квитанции о завершении от Обработчика
    bool ReleaseLetterFromWorker(Worker*);

    bool IsServiceCommandEnabled(const Service*, const std::string&);

    /* поместить сообщение во входящую очередь Службы */
    bool PushRequestToService(Service*, const std::string&, const std::string&, const std::string&);
    bool PushRequestToService(Service*, Letter*);

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
    Worker *PopWorker(const std::string&, WorkerState = ARMED);
    Worker *PopWorker(const Service*, WorkerState = ARMED);

    bool ClearWorkersForService(const Service*);
    bool ClearWorkersForService(const char*);
    bool ClearWorkersForService(const std::string&);

    // Удалить все Службы, при этом выполняется:
    // 1. Удаление всех Обработчиков, зарегистрированных за данной Службой
    // 2. Одновременное удаление Сообщений, присвоенных удаляемым Обработчикам
    bool ClearServices();

    void EnableServiceCommand (const std::string&, const std::string&);
    void EnableServiceCommand (const Service*, const std::string&);
    void DisableServiceCommand (const std::string&, const std::string&);
    void DisableServiceCommand (const Service*, const std::string&);

    /* Сохранение БД в виде XML */
    void MakeSnapshot(const char*);

  private:
    DISALLOW_COPY_AND_ASSIGN(DatabaseBrokerImpl);

    DatabaseImpl            *m_database;
    ServiceList             *m_service_list;
    Options                  m_opt;

    // Загрузка НСИ
    bool LoadDictionaries();
    // Удаление временных данных перед закрытием БД
    bool Cleanup();

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
                         broker_db::XDBService&);

    /* 
     * Вернуть новый Worker, построенный на основе прочитанных из БД данных
     * autoid_t - идентификатор Сервиса, содержащий данный Обработчик
     */
    MCO_RET LoadWorker(mco_trans_h,
                       broker_db::XDBWorker&,
                       Worker*&);

    /*
     * Заполнить указанный экземпляр Letter на основе своего состояния из БД
     */
    MCO_RET LoadLetter(mco_trans_h,
                       broker_db::XDBLetter&,
                       xdb::Letter*&);
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

    // Конвертировать состояние Службы из БД в состояние
    Service::State StateConvert(ServiceState);
    ServiceState StateConvert(Service::State);
};

} //namespace xdb
#endif
