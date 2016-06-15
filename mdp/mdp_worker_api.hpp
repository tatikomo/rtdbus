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
    enum { BROKER_ITEM = 0, SUBSCRIBER_ITEM = 1, DIRECT_ITEM = 2 };
    enum { SOCKETS_COUNT = DIRECT_ITEM+1 };

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
    void send(const std::string& serv_name, zmsg *&request);

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
    int  subscribe_to(const std::string&, const std::string&);
    int  ask_service_info(const std::string&, char*, int);
    bool service_info_by_name(const std::string&, ServiceInfo_t*&);
    
    //  ---------------------------------------------------------------------
    //  Set heartbeat delay
    void set_heartbeat (int heartbeat);

    //  ---------------------------------------------------------------------
    //  Set reconnect delay
    void set_reconnect (int reconnect);

    //  ---------------------------------------------------------------------
    //  Установить таймаут приема значений в милисекундах
    void set_recv_timeout(int msec);

    //  ---------------------------------------------------------------------
    //  wait for next request and get the address for reply.
    //  Таймаут по умолчанию
    zmsg * recv (std::string *&reply, int msec = HeartbeatInterval);

  protected:
    zmq::context_t   m_context;

  private:
    DISALLOW_COPY_AND_ASSIGN(mdwrk);

    std::string      m_broker_endpoint;
    std::string      m_service;
    // Связка "название Службы" <=> "информация о Службе", ключ хранится копией значения
    // Список Служб получать анализом данных от ask_service_info
    ServicesHash_t   m_services_info;
    // Точка подключения 
    const char      *m_direct_endpoint;
    zmq::socket_t   *m_worker;           //  Socket to broker
    zmq::socket_t   *m_direct;           //  Socket to subscribe on brokerless messages
    zmq::socket_t   *m_subscriber;
    //  Heartbeat management
    int64_t          m_heartbeat_at_msec;//  When to send HEARTBEAT
    size_t           m_liveness;         //  How many attempts left
    int              m_heartbeat_msec;   //  Heartbeat delay, msecs
    int              m_reconnect_msec;   //  Reconnect delay, msecs
    // Значения таймаутов, на передачу и на прием
    int              m_send_timeout_msec;
    int              m_recv_timeout_msec;
    //  Internal state
    bool             m_expect_reply;     //  Zero only at start
    // Хранилище из трёх сокетов для работы zmq::poll
    // [0] подключение к Брокеру
    // [1] подписка
    // [2] прямое подключение
    zmq::pollitem_t  m_socket_items[SOCKETS_COUNT];

    bool insert_service_info(const std::string&, ServiceInfo_t*&);
    // Вернуть строку подключения, если параметр = true - преобразовать для bind()
    const char     * getEndpoint(bool = false) const;
    void             update_heartbeat_sign();
};

} //namespace mdp

#endif
