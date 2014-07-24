#pragma once
#if !defined XDB_CORE_APPLICATION_HPP
#define XDB_CORE_APPLICATION_HPP

#include <string>
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

class Environment;

class Application
{
  public:
    Application(const char*);
    ~Application();

    const char* getAppName() const;
    const Error& setAppName(const char*);

    const Error& initialize();

    AppMode_t   getOperationMode() const;
    AppState_t  getOperationState() const;
    bool getOption(const std::string&, int&);
    void setOption(const char*, int);

    const Error& getLastError() const;
    void  setLastError(const Error&);
    void  clearError();

  private:
    DISALLOW_COPY_AND_ASSIGN(Application);
    char m_appli_name[IDENTITY_MAXLEN + 1];
    AppMode_t  m_mode;
    AppState_t m_state;
    Error    m_last_error;

  protected:
    Options  m_map_options;
};

} // namespace xdb

#endif

