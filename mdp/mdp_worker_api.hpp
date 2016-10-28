/*  =====================================================================
    mdp_worker_api.hpp

    Majordomo Protocol Worker API
    Implements the MDP/Worker spec at http://rfc.zeromq.org/spec:7.

    =====================================================================
*/

#pragma once
#ifndef MDP_WORKER_API_HPP_INCLUDED
#define MDP_WORKER_API_HPP_INCLUDED

#include <string>

#if defined HAVE_CONFIG_H
#include "config.h"
#endif

#include "mdp_zmsg.hpp"
#include "mdp_common.h"
#include "mdp_helpers.hpp"

namespace mdp {

class mdwrk
{
  public:
    // Первая запись - обязательна, подключение к Брокеру
    // Вторая запись - желательна, получение сообщений от других серверов подписки
    // Треться запись - не обязательна.
    enum { BROKER_ITEM = 0, SUBSCRIBER_ITEM = 1, DIRECT_ITEM = 2, PEER_ITEM = 3 };
    enum { SOCKETS_COUNT = PEER_ITEM + 1 };

    static const int HeartbeatInterval = HEARTBEAT_PERIOD_MSEC;
    //  ---------------------------------------------------------------------
    //  Constructor
    //  1) Строка подключения к Брокеру
    //  2) Название Службы
    //  3) Количество нитей ZMQ (default = 1)
    //  4) Необходимость создания сокета прямого подключения (default = false)
    mdwrk (const std::string&, const std::string&, int=1, bool=false);

    //  ---------------------------------------------------------------------
    //  Destructor
    virtual ~mdwrk ();

    //  ---------------------------------------------------------------------
    //  Send message to broker
    //  If no _msg is provided, creates one internally
    void send_to_broker(const char *command, const char* option, zmsg *_msg);

    //  ---------------------------------------------------------------------
    //  Send message to another Service
    bool send_to(const std::string& serv_name, zmsg *&request, ChannelType = DIRECT);

    //  ---------------------------------------------------------------------
    //  Connect or reconnect to broker
    void connect_to_broker ();

    // NB: для службы БДРВ заблокируем, т.к. эту заботу берет на себя DiggerProxy
    //  ---------------------------------------------------------------------
    //  Этот сокет используется для обмена "быстрыми" сообщениями, минуя Брокера
    void connect_to_world ();

    //  ---------------------------------------------------------------------
    //  Методы для работы с другими Службами
    //  ---------------------------------------------------------------------
    //  Подключиться к указанной службе на указанную группу
    bool subscribe_to(const std::string&, const std::string&);
    bool ask_service_info(const std::string&, ServiceInfo_t*&);
    bool service_info_by_name(const std::string&, ServiceInfo_t*&);
    
    //  ---------------------------------------------------------------------
    //  Set heartbeat delay
    void set_heartbeat (int heartbeat_msec);

    //  ---------------------------------------------------------------------
    //  Set reconnect delay
    void set_reconnect (int reconnect_msec);

    //  ---------------------------------------------------------------------
    //  Установить таймаут приема значений в милисекундах
    void set_recv_timeout(int msec);

    //  ---------------------------------------------------------------------
    //  wait for next request and get the address for reply.
    //  Параметры:
    //  1) Указатель на строку, содержащую идентификатор процесса-клиента
    //  2) Таймаут в милисекундах, по умолчанию равно периоду HEARTBEAT
    //  3) Адрес булевой переменной, содержащей признак завершения операции чтения по таймауту
    //
    //  Результат: Возвращается указатель на полученное сообщение.
    //  Если чтение завершено по таймауту, возвращается NULL,
    //  и значение булевой переменной становится равно TRUE.
    zmsg * recv (std::string *&reply, int msec = HeartbeatInterval, bool* = NULL);

  protected:
    zmq::context_t   m_context;

  private:
    DISALLOW_COPY_AND_ASSIGN(mdwrk);

    // Точка подключения к Брокеру
    std::string      m_broker_endpoint;
    // Название Службы
    std::string      m_service;
    // Связка "название Службы" <=> "информация о Службе", ключ хранится копией значения
    // Список Служб получать анализом данных от ask_service_info
    ServicesHash_t   m_services_info;
    // Точка подключения 
    const char      *m_direct_endpoint;
    zmq::socket_t   *m_worker;           //  Исходящий канал до Брокера
    zmq::socket_t   *m_peer;             //  Исходящий канал до других Служб = new zmq::socket_t(*m_context, ZMQ_DEALER)
    zmq::socket_t   *m_direct;           //  Входящий канал для получения прямых (brokerless) сообщений
    zmq::socket_t   *m_subscriber;       //  Входящий канал для получения сообщений о подписках
    //  Heartbeat management
    int              m_heartbeat_at_msec;//  When to send HEARTBEAT
    int              m_liveness;         //  How many attempts left
    int              m_heartbeat_msec;   //  Heartbeat delay, msecs
    int              m_reconnect_msec;   //  Reconnect delay, msecs
    // Значения таймаутов, на передачу и на прием.
    // Формат параметров - int (смотри спецификацию zmq::socket_t setsockopt())
    int              m_send_timeout_msec;
    int              m_recv_timeout_msec;
    //  Internal state
    bool             m_expect_reply;     //  Zero only at start
    // Хранилище из трёх сокетов для работы zmq::poll
    // [0] подключение к Брокеру
    // [1] подписка
    // [2] прямое подключение
    zmq::pollitem_t  m_socket_items[SOCKETS_COUNT];

    // Запомнить соответствие "Название Службы" <=> "Строка подключения"
    bool insert_service_info(const std::string&, ServiceInfo_t*&);
    // Вернуть строку подключения, если параметр = true - преобразовать для bind()
    const char     * getEndpoint(bool = false) const;
    void             update_heartbeat_sign();
    // Создать структуры для обмена с другими Службами
    void             create_peering_to_services();
    // Точка входа в разбор ответов Брокера
    void             process_internal_report(const std::string&, zmsg*);
    // Обработка ответа от Брокера с точкой подключения к другой Службе
    void             process_endpoint_info(zmsg*);
};

} //namespace mdp

#endif
