// =============================================================================================
// Родовой класс реализации интерфейса управления и получения данных для различных систем сбора
// 2016-06-14
//
// =============================================================================================
#ifndef EXCHANGE_SA_MODULE_INTERFACE_IMPL_H
#define EXCHANGE_SA_MODULE_INTERFACE_IMPL_H
#pragma once

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

// Общесистемные заголовочные файлы
#include <string>

// Служебные заголовочные файлы сторонних утилит
#include "zmq.hpp"

// Служебные файлы RTDBUS
#include "mdp_zmsg.hpp"
#include "exchange_config.hpp"
#include "exchange_config_sac.hpp"

class SMAD;

// =============================================================================================
// Набор состояний модуля
typedef enum {
  // Ещё не подключён, все в порядке
  STATUS_OK = 1,
  // Ещё не подключён, SMAD загружена
  STATUS_OK_SMAD_LOAD,                  // 2
  // Подключён, все в порядке
  STATUS_OK_CONNECTED,                  // 3
  // Не подключён, требуется переподключение
  STATUS_OK_NOT_CONNECTED,              // 4
  // Не подключён, выполняется останов
  STATUS_OK_SHUTTINGDOWN,               // 5
  // Подключён, требуется переподключение
  STATUS_FAIL_NEED_RECONNECTED,         // 6
  // Не подключён, переподключение не удалось
  STATUS_FAIL_TO_RECONNECT,             // 7
  // Нормальное завершение работы
  STATUS_OK_SHUTDOWN,                   // 8
  // Нет возможности продолжать работу из-за проблем с SMAD
  STATUS_FATAL_SMAD,                    // 9
  // Нет возможности продолжать работу из-за проблем с конфигурационными файлами
  STATUS_FATAL_CONFIG,                  // 10
  // Нет возможности продолжать работу из-за проблем с ОС
  STATUS_FATAL_RUNTIME                  // 11
} client_status_t;

// =============================================================================================

// =============================================================================================
// Базовый интерфейс взаимодействия с СС, безотносительно ее типа
// Реализация базовых функций интерфейса EGSA <=> SA
//
// TODO реализация таймеров для управления деятельностью СС
// К примеру, получив команду инициализации связи, модуль должен инициировать циклы:
// 1) таймаут подключения
// 2) получение данных общего сбора
// 3) получение второстепенных данных
// 4) передача данных
class SysAcqInterface {
  public:
    // Принимает код СС и ссылку на контекст (для связи с фронтендом)
    SysAcqInterface(const std::string&,
                    zmq::context_t&,
                    AcquisitionSystemConfig&);
    virtual ~SysAcqInterface();
    virtual void run() = 0;
    virtual void stop() = 0;
    virtual client_status_t connect() = 0;
    virtual client_status_t disconnect() = 0;

    // Точка входа обработки внешнего запроса 
    int handle_request(mdp::zmsg*, std::string*&);

    // Подготовительные действия перед работой - чтение конфигурации, инициализация,...
    virtual client_status_t prepare() { return STATUS_FATAL_RUNTIME; };
    // Элементарное действие, зависящее от текущего состояния
    virtual client_status_t quantum() { return STATUS_FATAL_RUNTIME; };
    client_status_t status()          { return m_status; };
    const std::string& service_name() { return m_service_name; };

  protected:
    // Обработчики инициированных самостоятельно циклов
    virtual void do_GENCONTROL() = 0;
    virtual void do_ACQSYSACQ() = 0;
    virtual void do_URGINFOS() = 0;
    virtual void do_INFOSACQ() = 0;

    client_status_t m_status;
    AcquisitionSystemConfig& m_config;
    sa_common_t     m_sa_common;
    SMAD           *m_smad;
    std::string     m_service_name;
    int             m_connection_restored;
    int             m_num_connection_try;
    // Контекст от фронтенда mdwrk 
    zmq::context_t &m_zmq_context;
    // сокет связи с фронтендом
    zmq::socket_t  *m_zmq_frontend;
};
// =============================================================================================

#endif

