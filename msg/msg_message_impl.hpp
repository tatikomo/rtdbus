#if defined HAVE_CONFIG_H
#include "config.h"
#endif

#include "proto/common.pb.h"
#include "google/protobuf/service.h"

#include "msg_message.hpp"

namespace msg
{

////////////////////////////////////////////////////////////////////////////////////////////////////
class DataImpl
{
  public:
    friend class Data;
   ~DataImpl();
    ::google::protobuf::Message* syncronize();
    ::google::protobuf::Message* instance();
    ::google::protobuf::Message* mutable_instance();
    bool valid() { return m_validity; };
    // Вернуть сериализованные данные
    const std::string& get_serialized();
    bool ParseFrom(const std::string&);

  private:
    DISALLOW_COPY_AND_ASSIGN(DataImpl);
    void set_initial_values();
    void valid(bool val)    { m_validity = val; };
    bool modified()         { return true == m_modified; };
    void modified(bool val) { m_modified = val; };

  protected:
    // Инициализация значениям по-умолчанию
    DataImpl(rtdbMsgType);
    // Инициализация указанными значениями
    DataImpl(rtdbMsgType, const std::string&);

    // Хранилище родового класса protobuf сериализованного сообщения известного типа
    // Фактически оно ссылается на созданный экземпляр соответствующего класса PROTOBUF:
    // RTDBM::AskLife, RTDBM::ExecResult, и т.п.
    ::google::protobuf::Message *m_protobuf_instance;
    // хранимый тип объекта
    rtdbMsgType m_type;
    // хранимое значение для десериализации
    std::string m_data;
    bool        m_validity;
    bool        m_modified;
    std::string m_pb_serialized;
};

////////////////////////////////////////////////////////////////////////////////////////////////////
class HeaderImpl
{
  public:
    friend class Header;
   ~HeaderImpl();
    RTDBM::Header& instance();
    RTDBM::Header& mutable_instance();
    bool valid() { return m_validity; };
    // Вернуть сериализованный заголовок
    const std::string& get_serialized();
    bool ParseFrom(const std::string&);

  private:
    void set_initial_values();
    void valid(bool val)    { m_validity = val; };
    bool modified()         { return true == m_modified; };
    void modified(bool val) { m_modified = val; };

  protected:
    HeaderImpl();
    HeaderImpl(const std::string&);

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

    // заголовок
    RTDBM::Header   m_instance;
    bool            m_validity;
    bool            m_modified;
    std::string     m_pb_serialized;
};
////////////////////////////////////////////////////////////////////////////////////////////////////

} // namespace msg

