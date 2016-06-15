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
//#include <map>
#include <string>

// Служебные заголовочные файлы сторонних утилит

// Служебные файлы RTDBUS
#include "exchange_config.hpp"

// =============================================================================================
// Набор состояний модуля
typedef enum {
  // Ещё не подключён, все в порядке
  STATUS_OK = 1,
  // Ещё не подключён, InternalSMAD загружена
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
  // Нет возможности продолжать работу из-за проблем с InternalSMAD
  STATUS_FATAL_SMAD,                    // 9
  // Нет возможности продолжать работу из-за проблем с конфигурационными файлами
  STATUS_FATAL_CONFIG,                  // 10
  // Нет возможности продолжать работу из-за проблем с ОС
  STATUS_FATAL_RUNTIME                  // 11
} client_status_t;

// =============================================================================================

class AcquisitionSystemConfig;
class InternalSMAD;

// =============================================================================================
class SysAcqInterface {
  public:
    SysAcqInterface(const std::string& filename) 
    : m_config_filename(filename),
      m_status(STATUS_OK_NOT_CONNECTED),
      m_config(NULL),
      m_sa_common(),
      m_smad(NULL),
      m_connection_reestablised(0),
      m_num_connection_try(0)
      {};
    virtual ~SysAcqInterface() {};
    // Подготовительные действия перед работой - чтение конфигурации, инициализация,...
    virtual client_status_t prepare() = 0;
    // Элементарное действие, зависящее от текущего состояния
    virtual client_status_t quantum() = 0;
    // Милисекунд ожидания между запросами
    int timewait() { return 0; };
    client_status_t status() { return m_status; };

  protected:
    std::string     m_config_filename;
    client_status_t m_status;
    AcquisitionSystemConfig* m_config;
    sa_common_t     m_sa_common;
    InternalSMAD   *m_smad;
    int             m_connection_reestablised;
    int             m_num_connection_try;
};
// =============================================================================================

#endif

