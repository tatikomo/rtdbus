#if !defined XDB_RTAP_APPLICATION_H_
#define XDB_RTAP_APPLICATION_H_
#pragma once

#include "config.h"
#include "xdb_rtap_error.hpp"

namespace xdb
{

class RtApplication
{
  public:
    typedef enum
    {
      MODE_UNKNOWN    =-1,
      MODE_LOCAL      = 0,
      MODE_REMOTE     = 1
    } RtAppMode;

    typedef enum
    {
      CONDITION_UNKNOWN =-1,
      CONDITION_GOOD    = 0,
      CONDITION_BAD     = 1
    } RtAppState;

    RtApplication(const char*);

    const char* getAppName() const;
    const RtError& setAppName(const char*);

    const char* getEnvName() const;
    const RtError& setEnvName(const char*);

    const RtError& initialize();

    RtAppMode getOperationMode() const;
    RtAppState getOperationState() const;
    const char* getOptions() const;
    const RtError& setOptions(const char*);

    const RtError& getLastError() const;

  private:
    DISALLOW_COPY_AND_ASSIGN(RtApplication);
    char m_appli_name[IDENTITY_MAXLEN + 1];
    char m_environment_name[IDENTITY_MAXLEN + 1];
    char m_options[IDENTITY_MAXLEN + 1];
    RtAppMode  m_mode;
    RtAppState m_state;
    RtError    m_error;
};

}

#endif

