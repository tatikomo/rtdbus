#pragma once
#ifndef XDB_RTAP_APPLICATION_HPP
#define XDB_RTAP_APPLICATION_HPP

#include <string>
#include <vector>
#include <map>

#if defined HAVE_CONFIG_H
#include "config.h"
#endif
#include "helper.hpp"
#include "xdb_common.hpp"
#include "xdb_impl_error.hpp"

namespace xdb {

// Описание хранилища опций в виде карты.
// Пара <символьный ключ> => <числовое значение>
#ifdef __SUNPRO_CC
typedef std::pair<std::string, int> Pair;
typedef std::map<std::string, int> Options;
#else
typedef std::pair<const std::string, int> Pair;
typedef std::map<const std::string, int> Options;
#endif
typedef Options::iterator OptionIterator;

class RtEnvironment;
class ApplicationImpl;

class RtApplication
{
  public:
    friend class RtEnvironment;

    RtApplication(const char*);
    ~RtApplication();

    const Error& initialize();

    const ::Options* getOptions() const;

    bool  getOption(const std::string&, int&);
    void  setOption(const char*, int);
    const char* getAppName() const;

    // Найти объект-среду по её имени
    RtEnvironment* isEnvironmentRegistered(const char* _env_name);
    // Загрузить среду с заданным именем, или среду по умолчанию
    RtEnvironment* loadEnvironment(const char* = NULL);
    // Подключиться к среде с заданным именем
    RtEnvironment* attach_to_env(const char*);

    const Error& getLastError() const;
    AppMode_t    getOperationMode() const;
    AppState_t   getOperationState() const;

  protected:
    ApplicationImpl *getImpl();

  private:
    DISALLOW_COPY_AND_ASSIGN(RtApplication);
    ApplicationImpl *m_impl;
    bool         m_initialized;
    std::vector<RtEnvironment*> m_env_list;
//    BitSet8      m_options;

    // Зарегистрировать в Приложении среду
    void         registerEnvironment(RtEnvironment*);
    // Вернуть экземпляр объекта, или создать новый для заданного имени
    // Если имя не задано явно, используется название Приложения
    // Необходимо удалить возвращенный экземпляр
    RtEnvironment* getEnvironment(const char* = NULL);
};

} // namespace xdb

#endif

