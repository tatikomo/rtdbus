#if !defined XDB_RTAP_ENVIRONMENT_H_
#define XDB_RTAP_ENVIRONMENT_H_
#pragma once

#include "config.h"
#include "xdb_rtap_common.h"
#include "mdp_letter.hpp"

namespace xdb
{
class RtDbConnection;

class RtEnvironment
{
  typedef enum
  {
    ERROR_NONE = 0,
    ERROR_ENVNAME_IS_EMPTY    = 1
  } RtError;

  public:
    RtEnvironment(const char*);
    ~RtEnvironment();
    // Получить код последней ошибки
    RtError getLastError() const;
    // Создать и вернуть новое подключение к указанной БД/среде
    RtDbConnection* createDbConnection();
    // Создать новое сообщение указанного типа
    mdp::Letter* createMessage(/* msgType */);
    // Вернуть имя подключенной ДБ/среды
    const char* getName() const;
    // Отправить сообщение адресату
    RtError sendMessage(mdp::Letter*);

  private:
    DISALLOW_COPY_AND_ASSIGN(RtEnvironment);
    char m_name[IDENTITY_MAXLEN + 1];
    RtError m_last_error;
};

}

#endif

