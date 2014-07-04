#if !defined XDB_RTAP_APPLICATION_H_
#define XDB_RTAP_APPLICATION_H_
#pragma once

#include <string>
#include <map>

#include "config.h"
#include "xdb_rtap_error.hpp"

namespace xdb
{

// Описание хранилища опций в виде карты.
// Пара <символьный ключ> => <числовое значение>
typedef std::pair<const std::string, int> Pair;
typedef std::map<const std::string, int> Options;
typedef std::map<const std::string, int>::iterator OptionIterator;

class RtEnvironment;

class RtApplication
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

    RtApplication(const char*);
    ~RtApplication();

    RtEnvironment* getEnvironment(const char*);

    const char* getAppName() const;
    const RtError& setAppName(const char*);

    const char* getEnvName() const;
    const RtError& setEnvName(const char*);

    const RtError& initialize();

    AppMode_t   getOperationMode() const;
    AppState_t  getOperationState() const;
    bool getOption(const std::string&, int&);
    void setOption(const char*, int);

    const RtError& getLastError() const;

  private:
    DISALLOW_COPY_AND_ASSIGN(RtApplication);
    char m_appli_name[IDENTITY_MAXLEN + 1];
    char m_environment_name[IDENTITY_MAXLEN + 1];
    AppMode_t  m_mode;
    AppState_t m_state;
    RtError    m_last_error;
    std::vector<RtEnvironment*> m_env_list;
    Options    m_map_options;
};

}

#endif

