#pragma once
#if !defined XDB_RTAP_ENVIRONMENT_HPP
#define XDB_RTAP_ENVIRONMENT_HPP

#include "config.h"

#include "xdb_core_common.h"
#include "xdb_core_error.hpp"
#include "mdp_letter.hpp"

namespace xdb {

class Environment;
class Application;
class RtConnection;

class RtEnvironment
{
  public:
    RtEnvironment(Application*, const char*);
    ~RtEnvironment();

    // Вернуть имя среды
    const char* getName() const;

    // Получить код последней ошибки
    const Error& getLastError() const;

    // Создать и вернуть новое подключение к указанной БД/среде
    RtConnection* createConnection();

    // Создать новое сообщение указанного типа
    mdp::Letter* createMessage(int);

    // Отправить сообщение адресату
    const Error& sendMessage(mdp::Letter*);

    // Получить состояние Среды
    EnvState_t getEnvState() const;

    // Загрузить содержимое БД данной среды из указанного XML файла
    const Error& LoadSnapshot(const char*);

    // Созранить БД в указанном файле в виде XML
    const Error& MakeSnapshot(const char*);

  private:
    Environment *m_impl;
};

} // namespace xdb

#endif
