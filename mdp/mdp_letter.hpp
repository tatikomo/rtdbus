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

    // Продоставление доступа только на чтение. Модификации данных учтены не будут
    // Доступ к служебному заголовку сообщения
    msg::Header& header() { return m_header_instance; }
    ::google::protobuf::Message* data();

    // Предоставление модифицируемой версии данных. После этого, перед любым методом, 
    // предоставляющим доступ к сериализованным данным, их сериализация должна быть повторена.
    ::google::protobuf::Message* mutable_data();
    msg::Header& mutable_header();

    // Создание тела сообщения на основе его пользовательского типа и сериализованного буфера.
    bool UnserializeFrom(const int, const std::string* = NULL);

    // Вернуть сериализованный заголовок
    const std::string& SerializedHeader();

    // Вернуть сериализованный буфер данных
    const std::string& SerializedData();

    // Установить значения полей "Отправитель" и "Получатель"
    void SetOrigin(const char*);
    void SetDestination(const char*);

    /*
     * Генерация нового системного идентификатора
     * NB: должна гарантироваться монотонно возрастающая уникальная последовательность 
     * цифр на протяжении ХХ часов для данного сервера.
     * TODO: определить критерии уникальности.
     */
    rtdbExchangeId GenerateExchangeId();

  private:
    msg::Header  m_header_instance; // содержит protobuf RTDBM::Header
    std::string  m_serialized_data;
    std::string  m_serialized_header;
    bool         m_initialized;
    bool         m_data_needs_reserialization;
    bool         m_header_needs_reserialization;
    // TODO нужно ли это поле? идентификатор есть в m_header_instance
    rtdbExchangeId m_exchange_id;
    std::string  m_source_procname;

    ::google::protobuf::Message *m_body_instance;
};

};

#endif

