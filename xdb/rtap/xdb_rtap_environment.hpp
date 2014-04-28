#if !defined XDB_RTAP_ENVIRONMENT_H_
#define XDB_RTAP_ENVIRONMENT_H_
#pragma once

#ifdef __cplusplus
extern "C" {
#include "mco.h"
#if (EXTREMEDB_VERSION >= 40) && USE_EXTREMEDB_HTTP_SERVER
#include "mcohv.h"
#endif
}
#endif

#include "config.h"
#include "xdb_rtap_common.h"
#include "xdb_rtap_error.hpp"
#include "mdp_letter.hpp"

namespace xdb
{

class RtApplication;
class RtDbConnection;
class RtCoreDatabase;

/*
 * Создание базы данных с помощью функции mco_db_open()
 * Подключения к базе возможны только после этого.
 * Подключения осуществляются с помощью вызовов mco_db_connect()
 * для ранее созданной БД.
 *
 * Вопрос: различные типы баз данных имеют уникальные функции словарей.
 * Как класс определит, какую словарную функцию использовать? Может быть 
 * задавать соответствие в RtApplication в виде хеша?
 * Пример:
 *   RTAP -> rtap_get_dictionary()
 *   ECH  -> ech_get_dictionary()
 * 23/10/2013: пока будем полагать, что всегда используется БД RTAP
 */

class RtEnvironment
{
  public:
    typedef enum
    {
      CONDITION_UNKNOWN = 0, // первоначальное состояние
      CONDITION_BAD     = 1, // критическая ошибка
      CONDITION_INIT    = 2, // среда проинициализирована, БД еще не открыта
      CONDITION_DB_OPEN = 3  // среда инициализирована, БД открыта
    } EnvState_t;

    RtEnvironment(RtApplication*, const char*);
    ~RtEnvironment();
    // Получить код последней ошибки
    const RtError& getLastError() const;
    // Создать и вернуть новое подключение к указанной БД/среде
    RtDbConnection* createDbConnection();
    // Создать новое сообщение указанного типа
    mdp::Letter* createMessage(/* msgType */);
    // Вернуть имя подключенной БД/среды
    const char* getName() const;
    // Отправить сообщение адресату
    RtError& sendMessage(mdp::Letter*);
    // Получить состояние Среды
    EnvState_t getEnvState() const { return m_state; }
    // Загрузить содержимое БД данной среды из указанного XML файла
    RtError& LoadSnapshot(const char*);
    // Созранить БД в указанном файле в виде XML
    RtError& MakeSnapshot(const char*);

  private:
    DISALLOW_COPY_AND_ASSIGN(RtEnvironment);
    char        m_name[IDENTITY_MAXLEN + 1];
    EnvState_t  m_state;
    RtError     m_last_error;
    RtCoreDatabase* m_database_impl;
    RtApplication*  m_appli;
#if EXTREMEDB_VERSION >= 40
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

    // Открыть БД без создания подключений
    RtError& openDB();
};

}

#endif

