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
    Letter(const rtdbMsgType, const std::string&, const std::string* = NULL);
    ~Letter();

    // Доступ к служебному заголовку сообщения
    msg::Header& header() { return m_header; }

    // Продоставление доступа только на чтение. Все модификации будут игнорированы.
    ::google::protobuf::Message* data() { return m_body_instance; }

    // Предоставление модифицируемой версии данных. После этого, перед любым методом, 
    // предоставляющим доступ к сериализованным данным, их сериализация должна быть повторена.
    ::google::protobuf::Message* mutable_data() { m_data_needs_reserialization = true; return m_body_instance; }

    // Создание тела сообщения на основе его пользовательского типа и сериализованного буфера.
    bool UnserializeFrom(const int, const std::string* = NULL);

    // Вернуть сериализованный заголовок
    std::string& SerializedHeader() { return m_serialized_header; }

    // Вернуть сериализованный буфер данных
    std::string& SerializedData();

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
    bool         m_data_needs_reserialization;
    rtdbExchangeId m_exchange_id;
    std::string  m_source_procname;

    ::google::protobuf::Message *m_body_instance;
};

};

#endif

