#if !defined MDP_LETTER_HPP
#define MDP_LETTER_HPP
#pragma once

#include <string>

#include "zmsg.hpp"
#include "msg_message.hpp"
#include "msg_common.h"

namespace mdp {

class Letter
{
  public:
    // На основе транспортного сообщения
    Letter(zmsg*);
    // На основе типа сообщения, получателя, и сериализованных данных
    Letter(const rtdbMsgType, const std::string&, const std::string&);
    ~Letter();

    // доступ к служебному заголовку сообщения
    /*const */msg::Header& header() { return m_header; }
    /*const */::google::protobuf::Message* data() { return m_body_instance; }
    // создание тела сообщения на основе его пользовательского типа и сериализоанного буфера
    bool UnserializeFrom(const int, const std::string&);
    std::string& SerializedHeader() { return m_serialized_header; }
    /*
     * Генерация нового системного идентификатора
     * NB: должна гарантироваться монотонно возрастающая уникальная последовательность 
     * цифр на протяжении ХХ часов для данного сервера.
     * TODO: определить критерии уникальности.
     */
    rtdbExchangeId GenerateExchangeId();

  private:
    msg::Header  m_header; // содержит protobuf RTDBM::Header
    zmsg        *m_message_instance;
    std::string  m_serialized_data;
    std::string  m_serialized_header;
    bool         m_initialized;
    rtdbExchangeId m_exchange_id;
    std::string  m_source_procname;

    ::google::protobuf::Message *m_body_instance;
};

};

#endif

