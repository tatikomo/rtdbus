#pragma once
#if !defined XDB_RTAP_APPLICATION_HPP
#define XDB_RTAP_APPLICATION_HPP

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
    const Error& getLastError() const;
    RtEnvironment* getEnvironment(const char*);
    AppMode_t  getOperationMode() const;
    AppState_t getOperationState() const;

  private:
    DISALLOW_COPY_AND_ASSIGN(RtApplication);
    Application *m_impl;
};

} // namespace xdb

#endif

