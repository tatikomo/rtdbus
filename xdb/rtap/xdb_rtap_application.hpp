#pragma once
#if !defined XDB_RTAP_APPLICATION_HPP
#define XDB_RTAP_APPLICATION_HPP

#include <string>
#include <vector>
#include <map>

#include "config.h"
#include "xdb_core_common.h"
#include "xdb_core_error.hpp"

namespace xdb {

// Описание хранилища опций в виде карты.
// Пара <символьный ключ> => <числовое значение>
typedef std::pair<const std::string, int> Pair;
typedef std::map<const std::string, int> Options;
typedef std::map<const std::string, int>::iterator OptionIterator;

class RtEnvironment;
class Application;

class RtApplication
{
  public:
    RtApplication(const char*);
    ~RtApplication();

    const Error& initialize();

    bool  getOption(const std::string&, int&);
    void  setOption(const char*, int);
    const char* getAppName() const;

    // Найти объект-среду по её имени
    RtEnvironment* isEnvironmentRegistered(const char* _env_name);
    // Загрузить среду с заданным именем, или среду по умолчанию
    RtEnvironment* loadEnvironment(const char* = NULL);

    const Error& getLastError() const;
    AppMode_t    getOperationMode() const;
    AppState_t   getOperationState() const;

  private:
    DISALLOW_COPY_AND_ASSIGN(RtApplication);
    Application *m_impl;
    bool         m_initialized;
    std::vector<RtEnvironment*> m_env_list;

    // Зарегистрировать в Приложении среду
    void         registerEnvironment(RtEnvironment*);
    // Вернуть экземпляр объекта, или создать новый для заданного имени
    // Если имя не задано явно, исполдьзуется название Приложения
    // Необходимо удалить возвращенный экземпляр
    RtEnvironment* getEnvironment(const char* = NULL);
};

} // namespace xdb

#endif

