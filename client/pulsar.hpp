#if !defined GEV_PULSAR_HPP
#define GEV_PULSAR_HPP
#pragma once

#include "mdp_client_async_api.hpp"

typedef enum {
  SENT_OUT      = 1,
  RECEIVED_IN   = 2
} Status;

class Pulsar: public mdp::mdcli
{
  public:
    Pulsar(std::string broker, int verbose);
    ~Pulsar() {};
    // Отправить сообщения
    void fire_messages();

  private:
    // Напрямую
    void fire_direct();
    // Через Брокера
    void fire_persistent();

    mdp::ChannelType m_channel;
};

#endif
