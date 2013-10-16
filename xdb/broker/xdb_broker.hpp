#if !defined XDB_DATABASE_BROKER_HPP
#define XDB_DATABASE_BROKER_HPP
#pragma once
#include <stdint.h>
#include <string>

#include "xdb_broker_base.hpp"
#include "xdb_broker_service.hpp"
#include "xdb_broker_worker.hpp"
#include "xdb_broker_letter.hpp"

namespace xdb {

class DatabaseBrokerImpl;
class ServiceList;
class Service;
class Worker;
class Letter;

class ServiceList
{
  public:
    // NB: экземпляр не должен создаваться вручную
    ServiceList() {};
    virtual ~ServiceList() {};

    virtual Service* first() = 0;
    virtual Service* last()  = 0;
    virtual Service* next()  = 0;
    virtual Service* prev()  = 0;
    // Получить количество зарегистрированных объектов
    virtual int size() const { return 0; }
    // Перечитать список Сервисов из базы данных
    virtual bool refresh()   = 0;
};

class DatabaseBroker : public Database
{

  public:
    DatabaseBroker();
    ~DatabaseBroker();

    bool Connect();

    /* Зарегистрировать Сервис */
    Service *AddService(const char*);
    Service *AddService(const std::string&);

    /* Удалить Сервис */
    bool RemoveService(const char*);
    /* Удалить Обработчик из всех связанных с ним таблиц БД */
    bool RemoveWorker(Worker*);
    /* Поместить в спул своего Сервиса */
    bool PushWorker(Worker*);
    /* Добавить нового Обработчика в спул Сервиса */
    Worker* PushWorkerForService(const std::string&, const std::string&);
    bool PushWorkerForService(Service*, Worker*); // TODO: delete me

    /* Проверить существование указанного Сервиса */
    bool IsServiceExist(const char*);
    /* Проверка разрешения использования указанной команды */
    bool IsServiceCommandEnabled(const Service*, const std::string&);
    /* получить доступ к текущему списку Сервисов */ 
    ServiceList* GetServiceList();
    /* назначить Сообщение данному Обработчику */
    bool AssignLetterToWorker(Worker*, Letter*);
    /* Очистить сообщение после получения квитанции о завершении от Обработчика */
    bool ReleaseLetterFromWorker(Worker*);

    /* поместить сообщение во входящую очередь Службы */
    bool PushRequestToService(Service*,
            const std::string&,
            const std::string&,
            const std::string&);
    bool PushRequestToService(Service*, Letter*);

    /* Вернуть экземпляр Сервиса, только если он существует в БД */
    Service *GetServiceByName(const char*);
    Service *GetServiceByName(const std::string&);
    Service *GetServiceById(int64_t);
    Service *GetServiceForWorker(const Worker*);

    /* Вернуть экземпляр Обработчика из БД. Не найден - вернуть NULL */
    Worker *GetWorkerByIdent(const char*);
    Worker *GetWorkerByIdent(const std::string&);

    /* Вернуть экземпляр Сервиса. Если он не существует в БД - создать */
    Service *RequireServiceByName(const char*);
    Service *RequireServiceByName(const std::string&);

    /* Выбрать свободного Обработчика и удалить его из спула своего Сервиса */
    Worker  *PopWorker(const std::string&);
    Worker  *PopWorker(Service*);

    /* Очистить спул Обработчиков указанного Сервиса */
    bool ClearWorkersForService(const char*);

    /* Очистить спул Обработчиков и всех Сервисов */
    bool ClearServices();

    /* Найти экземпляр Сообщения по паре Сервис/Обработчик */
    Letter* GetAssignedLetter(Worker*);
    /* Получить первое ожидающее обработки Сообщение */
    //bool GetWaitingLetter(Service*, Worker*, std::string&, std::string&);
    Letter* GetWaitingLetter(Service*);
    /* Установить новое состояние Обработчика */
    bool SetWorkerState(Worker*, Worker::State);
    /* Изменить состояние Сообщения */
    bool SetLetterState(Letter*, Letter::State);

    /* Разрешить исполнение Команды указанной Службе */
    void EnableServiceCommand (const std::string&, const std::string&);
    void EnableServiceCommand (const Service*, const std::string&);

    /* Запретить исполнение Команды указанной Службе */
    void DisableServiceCommand (const std::string&, const std::string&);
    void DisableServiceCommand (const Service*, const std::string&);

    /* Тестовый API сохранения базы */
    void MakeSnapshot(const char* = NULL);

  private:
    DISALLOW_COPY_AND_ASSIGN(DatabaseBroker);
    /* Ссылка на физическую реализацию интерфейса с БД */
    DatabaseBrokerImpl *m_impl;
};

} //namespace xdb

#endif
