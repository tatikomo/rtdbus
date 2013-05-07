#if !defined XDB_DATABASE_BROKER_HPP
#define XDB_DATABASE_BROKER_HPP
#pragma once
#include <stdint.h>
#include <string>
#include "xdb_database.hpp"

class XDBDatabaseBrokerImpl;
class Service;
class Worker;
class Letter;

class XDBDatabaseBroker : public XDBDatabase
{
  public:
    XDBDatabaseBroker();
    ~XDBDatabaseBroker();

    bool Open();

    /* Зарегистрировать Сервис */
    bool AddService(const char*);

    /* Удалить Сервис */
    bool RemoveService(const char*);
    /* Удалить Обработчик из всех связанных с ним таблиц БД */
    bool RemoveWorker(Worker*);

    /* Проверить существование указанного Сервиса */
    bool IsServiceExist(const char*);

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
    Service *RequireServiceByName(std::string&);

    /* Выбрать свободного Обработчика и удалить его из спула данного Сервиса */
    Worker  *PopWorkerForService(const char*);
    Worker  *PopWorkerForService(Service*);

    /* Поместить нового Обработчика в спул данного Сервиса */
    bool PushWorkerForService(Service*, Worker*);

    /* Очистить спул Обработчиков указанного Сервиса */
    bool ClearWorkersForService(const char*);

    /* Очистить спул Обработчиков и всех Сервисов */
    bool ClearServices();

    /* Получить первое ожидающее обработки Сообщение */
    Letter* GetWaitingLetter(Service*, Worker*);

    /* Разрешить исполнение Команды указанной Службе */
    void EnableServiceCommand (const std::string&, const std::string&);
    void EnableServiceCommand (const Service*, const std::string&);

    /* Запретить исполнение Команды указанной Службе */
    void DisableServiceCommand (const std::string&, const std::string&);
    void DisableServiceCommand (const Service*, const std::string&);

  private:
    /* Ссылка на физическую реализацию */
    XDBDatabaseBrokerImpl *m_impl;
};

#endif
