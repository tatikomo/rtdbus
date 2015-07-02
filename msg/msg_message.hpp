#pragma once
#ifndef MSG_MESSAGE_HPP
#define MSG_MESSAGE_HPP

#include <string>

#include "config.h"
#include "msg_common.h"

#include "mdp_zmsg.hpp"

namespace msg {

class Header;
class HeaderImpl;
class Data;
class DataImpl;
class Letter;
class MessageFactory;

////////////////////////////////////////////////////////////////////////////////////////////////////
class Letter
{
  public:
    friend class MessageFactory;

    // Деструктор
    virtual ~Letter();

    // вывод основной информации в консоль
    virtual void dump();

    Header  *header();
    Data    *data();

    // Установить значения полей "Отправитель" и "Получатель"
    void set_origin(const char*);
    void set_destination(const char*);
    void set_destination(const std::string&);

    // признак корректности данных объекта
    bool valid();

  private:
    DISALLOW_COPY_AND_ASSIGN(Letter);

    /*
     * Генерация нового системного идентификатора
     * NB: должна гарантироваться монотонно возрастающая уникальная последовательность 
     * цифр на протяжении ХХ часов для данного сервера.
     * TODO: определить критерии уникальности.
     */
    rtdbExchangeId generate_next_exchange_id();

  protected:
    // Создание нового сообщения
    Letter(const rtdbMsgType, rtdbExchangeId = 0);
    // Создание нового сообщения с уже десериализованым заголовком
    Letter(Header*, const std::string&);
    // Создание нового сообщения на основе фреймов заголовка и нагрузки
    Letter(const std::string&, const std::string&);

    // Хранилище объекта-заголовка
    Header *m_header;
    // Хранилище объекта-тела сообщения
    Data   *m_data;

    // Вернуть признак того, была ли модификация данных с момента последней десериализации
    bool modified();
    void modified(bool);
};

////////////////////////////////////////////////////////////////////////////////////////////////////
// Служебный заголовок сообщения
class Header
{
  public:
    friend class Letter;
    // Создать все по умолчанию
    Header();
    // входной параметр - фрейм заголовка из zmsg
    Header(const std::string&);
    ~Header();

    // Вернуть сериализованный заголовок
    const std::string& get_serialized();
    // признак корректности данных объекта
    bool valid();

    bool ParseFrom(const std::string&);

    // RO-доступ к служебным полям
    uint32_t       protocol_version() const;
    rtdbExchangeId exchange_id() const;
    rtdbExchangeId interest_id() const;
    rtdbPid        source_pid() const;
    const std::string&   proc_dest() const;
    const std::string&   proc_origin() const;
    rtdbMsgType    sys_msg_type() const;
    rtdbMsgType    usr_msg_type() const;
    uint64_t       time_mark() const;

    // Запись в служебные поля
    void set_protocol_version(uint32_t);
    void set_exchange_id(rtdbExchangeId);
    void set_interest_id(rtdbExchangeId);
    // установка pid проводится автоматически
    void set_proc_dest(const std::string&);
    void set_proc_origin(const std::string&);
    void set_proc_origin(const char*);
    void set_sys_msg_type(rtdbMsgType);
    void set_usr_msg_type(rtdbMsgType);
    void set_time_mark(uint64_t);

    // Вернуть фактический класс PROTOBUF, хранящий данное сообщение 
    HeaderImpl  *impl() const;

  private:
    DISALLOW_COPY_AND_ASSIGN(Header);
    HeaderImpl  *m_impl;
};


////////////////////////////////////////////////////////////////////////////////////////////////////
// Тело сообщения
class Data
{
  public:
    friend class Letter;
    // Создать все по умолчанию для данного типа
    Data(rtdbMsgType);
    // входной параметр - тип сообщения и фрейм zmq с данными
    Data(rtdbMsgType, const std::string&);
   ~Data();

    // Вернуть сериализованные данные
    const std::string& get_serialized();

    // признак того, была ли модификация данных
    bool modified();
    void modified(bool);
    // Признак корретности состояния объекта:
    //   был успешно инициализирован
    //   десериализация успешна
    bool valid();

    bool ParseFrom(const std::string&);

    // Вернуть фактический класс PROTOBUF, хранящий данное сообщение 
    DataImpl*    impl() const;

  private:
    DISALLOW_COPY_AND_ASSIGN(Data);
    DataImpl    *m_impl;
};


////////////////////////////////////////////////////////////////////////////////////////////////////
class MessageFactory
{
  public:
    // Инициализация фабрики создания сообщений названием своего процесса
    MessageFactory(const char*);
   ~MessageFactory();

    // Вернуть новое сообщение указанного типа
    Letter* create(rtdbMsgType);
    // Вернуть новое сообщение на основе прочитанных из сокета zmq фреймов
    Letter* create(mdp::zmsg*);

    // Создание сообщения на основе его пользовательского типа и сериализованного буфера.
    //Letter* unserialize(rtdbMsgType, const std::string&);

    // Создание сообщения на основе фреймов заголовка и сериализованных данных.
    Letter* unserialize(const std::string&, const std::string&);

  private:
    DISALLOW_COPY_AND_ASSIGN(MessageFactory);
    unsigned int    m_version_message_system;
    unsigned int    m_version_rtdbus;
    pid_t           m_pid;
    // Заголовок со значениями "по-умолчанию" для всех сообщений.
    // Изменяется только идентификатор обмена exchange_id
    std::string     m_default_serialized_header;
    // Header          m_default_pb_header;
    // Название собственного процесса, подставляется при отправке в каждое сообщение
    std::string     m_source_procname; //[SERVICE_NAME_MAXLEN + 1];
    static rtdbExchangeId  m_exchange_id;
};



} // namespace msg

#endif

