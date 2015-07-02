#if !defined GEV_PULSAR_HPP
#define GEV_PULSAR_HPP
#pragma once

#include <string>
#include <map>

#include "mdp_client_async_api.hpp"

typedef enum {
  SENT_OUT      = 1,
  RECEIVED_IN   = 2
} Status;

typedef std::map  <const std::string, rtdbMsgType> MessageTypesHash_t;
typedef std::pair <const std::string, rtdbMsgType> MessageTypesHashPair_t;
typedef MessageTypesHash_t::iterator MessageTypesHashIterator_t;

class Pulsar: public mdp::mdcli
{
  public:
    Pulsar(const char* broker, int verbose);
    ~Pulsar();
    // Отправить сообщения
    void fire_messages();
    // вернуть тип сообщения по его названию
    rtdbMsgType getMsgTypeByName(std::string&);

  private:
    // Напрямую
    void fire_direct();
    // Через Брокера
    void fire_persistent();
    // Инициализация структур
    void init();

    mdp::ChannelType m_channel;
    MessageTypesHash_t m_msg_type_hash;
};

#endif
