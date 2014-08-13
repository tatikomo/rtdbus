#pragma once
#ifndef XDB_IMPL_ENVIRONMENT_HPP
#define XDB_IMPL_ENVIRONMENT_HPP

#if defined HAVE_CONFIG_H
#include "config.h"
#endif
#include "xdb_impl_common.hpp"
#include "xdb_impl_error.hpp"

namespace xdb {

class ApplicationImpl;
class ConnectionImpl;

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

class EnvironmentImpl
{
  public:
    EnvironmentImpl(ApplicationImpl*, const char*);
    ~EnvironmentImpl();

    // Получить код последней ошибки
    const Error& getLastError() const;
    void  setLastError(const Error&);
    void  clearError();

    // Создать и вернуть новое подключение к указанной БД/среде
    ConnectionImpl* getConnection();
    // Вернуть имя подключенной БД/среды
    const char* getName() const;
    // Вернуть имя Приложения
    const char* getAppName() const;
    // Получить состояние Среды
    EnvState_t getEnvState() const { return m_state; }
    void       setEnvState(EnvState_t);
    // Загрузить содержимое БД данной среды из указанного XML файла
    const Error& LoadSnapshot(const char*, const char* = NULL);
    // Созранить БД в указанном файле в виде XML
    const Error& MakeSnapshot(const char* = NULL);

  protected:
    void setError(Error&);

  private:
    DISALLOW_COPY_AND_ASSIGN(EnvironmentImpl);
    char        m_name[IDENTITY_MAXLEN + 1];
    EnvState_t  m_state;
    Error     m_last_error;
    ApplicationImpl  *m_appli;
    ConnectionImpl   *m_conn;

    // Открыть БД без создания подключений
    Error& openDB();
};

} // namespace xdb

#endif

