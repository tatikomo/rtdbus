#pragma once
#ifndef XDB_IMPL_ENVIRONMENT_HPP
#define XDB_IMPL_ENVIRONMENT_HPP

#if defined HAVE_CONFIG_H
#include "config.h"
#endif

#include "xdb_impl_error.hpp"

namespace xdb {

class ApplicationImpl;
class ConnectionImpl;

/*
 * 1. mco_db_open: Создание базы данных
 * 2. mco_db_connect: Подключение к базе для ранее созданной БД
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
    //ConnectionImpl* getConnection();
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
    char              m_name[IDENTITY_MAXLEN + 1];
    EnvState_t        m_state;
    Error             m_last_error;
    ApplicationImpl  *m_appli;
    ConnectionImpl   *m_conn;
};

} // namespace xdb

#endif

