#pragma once
#ifndef XDB_RTAP_ENVIRONMENT_HPP
#define XDB_RTAP_ENVIRONMENT_HPP

#include "config.h"

#include "xdb_impl_common.hpp"
#include "xdb_impl_error.hpp"
#include "mdp_letter.hpp"

namespace xdb {

class EnvironmentImpl;
class RtConnection;
class RtDatabase;
class RtApplication;

class RtEnvironment
{
  public:
    friend class RtApplication;
    friend class RtConnection;

    RtEnvironment(RtApplication*, const char*);
    ~RtEnvironment();

    // Вернуть имя среды
    const char* getName() const;

    // Получить код последней ошибки
    const Error& getLastError() const;

    // Создать и вернуть новое подключение к указанной БД/среде
    RtConnection* getConnection();

    // Создать новое сообщение указанного типа
    mdp::Letter* createMessage(int);

    // Отправить сообщение адресату
    const Error& sendMessage(mdp::Letter*);

    // Получить состояние Среды
    EnvState_t getEnvState() const;

    // Изменить состояние Среды
    void setEnvState(EnvState_t);

    // Загрузить содержимое БД данной среды из указанного XML файла
    const Error& LoadSnapshot(const char* = NULL);

    // Созранить БД в указанном файле в виде XML
    const Error& MakeSnapshot(const char*);

    // Запустить исполнение
    const Error& Start();

    // Завершить исполнение
    const Error& Shutdown(EnvShutdownOrder_t);

  protected:
    const Error& LoadDictionary();

  private:
    EnvironmentImpl *m_impl;
    RtApplication   *m_appli;
    RtConnection    *m_conn;
    RtDatabase      *m_database;
};

} // namespace xdb

#endif
