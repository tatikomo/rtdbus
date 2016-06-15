#ifndef SERVICE_SYSACQ_MODULE_H
#define SERVICE_SYSACQ_MODULE_H
#pragma once

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

// Общесистемные заголовочные файлы

// Служебные заголовочные файлы сторонних утилит

// Служебные файлы RTDBUS
#include "mdp_worker_api.hpp"
//#include "exchange_sysacq_intf.hpp"

// Ссылка на общий интерфейс систем сбора
class SysAcqInterface;

namespace mdp {

class SystemAcquisitionModule : public mdwrk {
  public:
    SystemAcquisitionModule(const std::string&, const std::string&, const std::string&);
   ~SystemAcquisitionModule();
    int run();
    int handle_request(mdp::zmsg*, std::string*&);

  private:
    SysAcqInterface* m_impl;
    int m_status;
    // Фабрика сообщений
    msg::MessageFactory *m_message_factory;

    int handle_asklife(msg::Letter*, std::string*);
    int handle_stop(msg::Letter*, std::string*);
};

}; // end namespace

#endif

