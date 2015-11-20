#pragma once
#ifndef XDB_IMPL_APPLICATION_HPP
#define XDB_IMPL_APPLICATION_HPP

#include <string>

#if defined HAVE_CONFIG_H
#include "config.h"
#endif

#include "helper.hpp"
#include "xdb_common.hpp"
#include "xdb_impl_error.hpp"

namespace xdb {

class ApplicationImpl
{
  public:
    ApplicationImpl(const char*);
    ~ApplicationImpl();

    const char* getAppName() const;
    const Error& setAppName(const char*);

    const Error& initialize();

    AppMode_t   getOperationMode() const;
    AppState_t  getOperationState() const;
    bool getOption(const std::string&, int&);
    void setOption(const char*, int);
    Options* getOptions();

    const Error& getLastError() const;
    void  setLastError(const Error&);
    void  clearError();
    // Проверка присутствия в системе зарегистрированного экземпляра указанной БД
    bool  database_instance_presence(const char*);

  private:
    DISALLOW_COPY_AND_ASSIGN(ApplicationImpl);
    char       m_appli_name[IDENTITY_MAXLEN + 1];
    AppMode_t  m_mode;
    AppState_t m_state;
    Error      m_last_error;

  protected:
    Options  m_map_options;
};

} // namespace xdb

#endif

