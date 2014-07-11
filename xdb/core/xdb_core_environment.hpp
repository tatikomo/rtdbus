#pragma once
#if !defined XDB_CORE_ENVIRONMENT_HPP
#define XDB_CORE_ENVIRONMENT_HPP

#include "config.h"
#include "xdb_core_common.h"
#include "xdb_core_error.hpp"

namespace xdb {

class Application;
class Connection;
class CoreDatabase;

/*
 * Создание базы данных с помощью функции mco_db_open()
 * Подключения к базе возможны только после этого.
 * Подключения осуществляются с помощью вызовов mco_db_connect()
 * для ранее созданной БД.
 *
 * Вопрос: различные типы баз данных имеют уникальные функции словарей.
 * Как класс определит, какую словарную функцию использовать?
 * Может быть задавать соответствие в Application в виде хеша?
 *
 * Пример:
 *   RTAP -> rtap_get_dictionary()
 *   ECH  -> ech_get_dictionary()
 *
 * 23/10/2013: пока будем полагать, что всегда используется БД RTAP
 */

class Environment
{
  public:
    Environment(Application*, const char*);
    ~Environment();

    // Получить код последней ошибки
    const Error& getLastError() const;
    void  setLastError(const Error&);
    void  clearError();

    // Создать и вернуть новое подключение к указанной БД/среде
    Connection* createConnection();
    // Вернуть имя подключенной БД/среды
    const char* getName() const;
    // Получить состояние Среды
    EnvState_t getEnvState() const { return m_state; }
    // Загрузить содержимое БД данной среды из указанного XML файла
    const Error& LoadSnapshot(const char* = NULL);
    // Созранить БД в указанном файле в виде XML
    const Error& MakeSnapshot(const char* = NULL);

  protected:
    void setError(Error&);

  private:
    DISALLOW_COPY_AND_ASSIGN(Environment);
    char        m_name[IDENTITY_MAXLEN + 1];
    EnvState_t  m_state;
    Error     m_last_error;
    CoreDatabase* m_database_impl;
    Application*  m_appli;

    // Открыть БД без создания подключений
    Error& openDB();
};

} // namespace xdb

#endif

