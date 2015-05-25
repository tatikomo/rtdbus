#pragma once
#ifndef MSG_MESSAGE_HPP
#define MSG_MESSAGE_HPP

#include "msg_common.h"
#include "proto/common.pb.h"

namespace msg {

/*
 * Базовые классы сообщений, передаваемых в сети RTDBUS
 */

// Служебный заголовок сообщения
class Header
{
  public:
    // Создать все по умолчанию
    Header();
    // входной параметр - фрейм заголовка из zmsg
    Header(const std::string&);
    ~Header();

    bool ParseFrom(const std::string&);
    RTDBM::Header& instance() { return m_header_instance; }

    // RO-доступ к служебным полям
    uint32_t       get_protocol_version() const;
    rtdbExchangeId get_exchange_id() const;
    rtdbExchangeId get_interest_id() const;
    rtdbPid        get_source_pid() const;
    const std::string&   get_proc_dest() const;
    const std::string&   get_proc_origin() const;
    rtdbMsgType    get_sys_msg_type() const;
    rtdbMsgType    get_usr_msg_type() const;

    const std::string&   get_serialized();

    // RW-доступ к пользовательским полям
    void            set_usr_msg_type(int16_t);

  private:
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

// Тело сообщения
class Payload
{
  public:
    Payload();
    // входной параметр - фрейм заголовка из zmsg
    Payload(const std::string&);
    virtual ~Payload();
    const std::string&   get_serialized();

  private:
    std::string     pb_serialized;
};

// Сообщение целиком
class Message
{
  public:
    // Создание нового соощения
    Message(rtdbMsgType, uint32_t);
    // Восстановление сообщения на основе фреймов заголовка и нагрузки
    Message(const std::string&, const std::string&);
    virtual ~Message();
    const Header*   get_head();
    const Payload*  get_body();
    virtual rtdbMsgType type() = 0;

    virtual void dump();

  protected:
    RTDBM::Header  m_pb_header;
    Header  *m_header;
    Payload *m_payload;
    // системный тип сообщения
    rtdbMsgType m_system_type;
    // пользовательский тип сообщения
    rtdbMsgType m_user_type;
    // идентификатор обмена 
    rtdbExchangeId m_exchange_id;
    // идентификатор запроса в рамках обмена
    rtdbExchangeId m_interest_id;

  private:
};

class AskLife : public Message
{
  public:
    AskLife();
    AskLife(rtdbExchangeId);
    AskLife(const std::string&, const std::string&);
   ~AskLife();
    rtdbMsgType type() { return m_user_type; };

    int status();

  private:
    RTDBM::AskLife  m_pb_impl;
};

class ExecResult : public Message
{
  public:
    ExecResult();
    ExecResult(rtdbExchangeId);
    ExecResult(const std::string&, const std::string&);
   ~ExecResult();
   rtdbMsgType type() { return m_user_type; };

  private:
};


class MessageFactory
{
  public:
    MessageFactory();
   ~MessageFactory();

    // вернуть новое сообщение указанного типа
    Message* create(rtdbMsgType);

  private:
    int     m_version_message_system;
    int     m_version_rtdbus;
    pid_t   m_pid;
    rtdbExchangeId m_exchange_id;
};


} //namespace msg

#endif

