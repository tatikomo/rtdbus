#ifndef SERVICE_EXCHANGE_MODBUS_CLIENT_H
#define SERVICE_EXCHANGE_MODBUS_CLIENT_H
#pragma once

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

// Общесистемные заголовочные файлы

// Служебные заголовочные файлы сторонних утилит

// Служебные файлы RTDBUS
#include "mdp_worker_api.hpp"

class RTDBUS_Modbus_client;

namespace mdp {

class MBusClient : public mdwrk {
  public:
    MBusClient(const std::string& broker_endpoint, const std::string& service);
   ~MBusClient();
    int run();

  private:
    RTDBUS_Modbus_client* m_impl;
    int m_status;
};

}; // end namespace

#endif

