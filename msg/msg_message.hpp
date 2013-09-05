#if !defined MSG_MESSAGE_HPP
#define MSG_MESSAGE_HPP
#pragma once

#include <google/protobuf/stubs/common.h>
#include "msg_common.h"
#include "proto/common.pb.h"

class Payload;

/*
 * Базовые классы сообщений, передаваемых в сети RTDBUS
 */

// Служебный заголовок сообщения
class RTDBUS_MessageHeader
{
  public:
    // входной параметр - фрейм заголовка из zmsg
    RTDBUS_MessageHeader(const std::string&);
    ~RTDBUS_MessageHeader();

    // RO-доступ к служебным полям
    int8_t         get_protocol_version();
    rtdbExchangeId get_exchange_id();
    rtdbPid        get_source_pid();
    const std::string&    get_proc_dest();
    const std::string&    get_proc_origin();
    rtdbMsgType    get_sys_msg_type();
    rtdbMsgType    get_usr_msg_type();

    const std::string&   get_serialized();

    // RW-доступ к пользовательским полям
    void            set_usr_msg_type(int16_t);

  private:
    RTDBUS_MessageHeader();
    RTDBM::Header* dump() { return &m_header_instance; }
#if 0
    /* 
     * Служебные поля заполняются автоматически, [RO]
     * Пользовательские поля заполняются вручную, [RW]
     */
    uint8_t         m_protocol_version; /* [служебный] версия протокола */
    rtdbExchangeId  m_exchange_id;  /* [служебный] идентификатор сообщения */
    rtdbPid         m_source_pid;   /* [служебный] системый идентификатор процесса-отправителя сообщения */
    rtdbProcessId   m_proc_dest;    /* [служебный] название процесса-получателя */
    rtdbProcessId   m_proc_origin;  /* [служебный] название процесса-создателя сообщения */
    rtdbMsgType     m_sys_msg_type; /* [служебный] общесистемный тип сообщения */
    rtdbMsgType     m_usr_msg_type; /* [пользовательский] клиентский тип сообщения */
#endif

    std::string     pb_serialized;
    // Сам заголовок
    RTDBM::Header   m_header_instance;
};

class RTDBUS_MessageBody
{
  public:
    // входные параметры - заголовок и фрейм тела сообщения из zmsg
    RTDBUS_MessageBody(const RTDBUS_MessageHeader*, const std::string&);
    virtual ~RTDBUS_MessageBody();
    rtdbMsgType          get_type();
    const std::string&   get_serialized();
    RTDBUS_MessageHeader* header() { return m_header_instance; }
    ::google::protobuf::Message *dump() { return m_body_instance; }

  private:
    RTDBUS_MessageBody();
    RTDBUS_MessageHeader        *m_header_instance;
    std::string                  pb_serialized;
    ::google::protobuf::Message *m_body_instance;
};

class RTDBUS_MessageFactory
{
  public:
    RTDBUS_MessageFactory();
    ~RTDBUS_MessageFactory();

    // создать экземпляр заданного клиентского типа
    static ::google::protobuf::Message *create(rtdbMsgType);
};

#endif
