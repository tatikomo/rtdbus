#ifndef EXCHANGE_SMAD_IMPL_H
#define EXCHANGE_SMAD_IMPL_H
#pragma once

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

// Общесистемные заголовочные файлы
//#include <assert.h>

// Служебные заголовочные файлы сторонних утилит
#include "sqlite3.h"

// Служебные файлы RTDBUS
#include "exchange_config.hpp"

// ===========================================================================================
// Доступ к БД кратковременного хранения данных, поступивших от СС
// На период отсутствия связи между модулем сбора и верхним уровнем данные кратковременно
// накапливаются в локальной БД SMAD, для последующей передачи на верхний уровень.
class SMAD
{
  public:
    // текущее состояние подключения
    typedef enum { STATE_OK = 1, STATE_DISCONNECTED = 2, STATE_ERROR = 3 } connection_state_t;

    // ПОдключиться к указанному снимку БД
    SMAD(const char*);
   ~SMAD();

    // Подключиться к указанной таблице SMAD
    connection_state_t connect(const char*);
    connection_state_t state() { return m_state; };
    // Занести значение в SMAD
    int push(const sa_parameter_info_t&);
    // Удалить из SMAD уже переданные наружу значения
    int purge();
    // Установить первоначальное значения параметра
    int setup_parameter(const sa_parameter_info_t&);
    // Изменить/вернуть ежим журналирования БД для ускорения занесения в неё данных
    void accelerate(bool);

  private:
    DISALLOW_COPY_AND_ASSIGN(SMAD);
    // Создать таблицу по заданному имени и SQL-выражению
    bool create_table(const char*, const char*);
    bool drop_table(const char*);
    // Получить идентификатор системы сбора с указанным именем
    bool get_sa_reference(char*, int&);

    // -------------------------------------------------------------------
    static const char *s_SMAD_DESCRIPTION_TABLENAME;
    static const char *s_SQL_CREATE_SMAD_TEMPLATE;
    static const char *s_SQL_INSERT_SMAD_TEMPLATE;
    static const char *s_SQL_CREATE_SA_TEMPLATE;
    static const char *s_SQL_INSERT_SA_HEADER_INT_TEMPLATE;
    static const char *s_SQL_INSERT_SA_HEADER_FLOAT_TEMPLATE;
    static const char *s_SQL_INSERT_SA_VALUES_INT_TEMPLATE;
    static const char *s_SQL_INSERT_SA_VALUES_FLOAT_TEMPLATE;

    sqlite3* m_db;
    connection_state_t  m_state;
    char*    m_db_err;
    char*    m_snapshot_filename;
    char*    m_sa_name;
    int      m_sa_reference;
};

#endif

