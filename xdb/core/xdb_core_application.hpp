#pragma once
#if !defined XDB_CORE_APPLICATION_HPP
#define XDB_CORE_APPLICATION_HPP

#include <string>
#include <vector>
#include <map>

#include "config.h"
#include "xdb_core_error.hpp"

namespace xdb {
namespace core {

// Описание хранилища опций в виде карты.
// Пара <символьный ключ> => <числовое значение>
typedef std::pair<const std::string, int> Pair;
typedef std::map<const std::string, int> Options;
typedef std::map<const std::string, int>::iterator OptionIterator;

class Environment;

class Application
{
  public:
    typedef enum
    {
      MODE_UNKNOWN    =-1,
      MODE_LOCAL      = 0,
      MODE_REMOTE     = 1
    } AppMode_t;

    typedef enum
    {
      CONDITION_UNKNOWN =-1,
      CONDITION_GOOD    = 0,
      CONDITION_BAD     = 1
    } AppState_t;

    Application(const char*);
    virtual ~Application();

    Environment* getEnvironment(const char*);

    const char* getAppName() const;
    const Error& setAppName(const char*);

    const char* getEnvName() const;
    const Error& setEnvName(const char*);

    virtual const Error& initialize();

    AppMode_t   getOperationMode() const;
    AppState_t  getOperationState() const;
    virtual bool getOption(const std::string&, int&);
    virtual void setOption(const char*, int);

    const Error& getLastError() const;
    void  setLastError(const Error&);
    void  clearError();

  private:
    DISALLOW_COPY_AND_ASSIGN(Application);
    char m_appli_name[IDENTITY_MAXLEN + 1];
    char m_environment_name[IDENTITY_MAXLEN + 1];
    AppMode_t  m_mode;
    AppState_t m_state;
    Error    m_last_error;
  protected:
    std::vector<Environment*> m_env_list;
    Options    m_map_options;
};

} // namespace core
} // namespace xdb

#endif

