#ifndef EXCHANGE_SMAD_INTERNAL_H
#define EXCHANGE_SMAD_INTERNAL_H
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
class InternalSMAD
{
  public:
    // ПОдключиться к указанному снимку БД
    InternalSMAD(const char*);
   ~InternalSMAD();

    // Подключиться к указанной таблице InternalSMAD, создав новую запись для указанной СС
    // NB: используется модулем СС
    smad_connection_state_t connect(const char*, gof_t_SacNature);
    // Подключиться к указанной СС, не создавая новых записей
    // NB: используется EGSA
    smad_connection_state_t attach(const char*, gof_t_SacNature);
    smad_connection_state_t state() { return m_state; };
    gof_t_SacNature nature() { return m_sa_nature; };
    // Занести значение в InternalSMAD
    int push(const sa_parameter_info_t&);
    // Прочитать измененные значения
    int pop(sa_parameters_t&);
    // Удалить из InternalSMAD уже переданные наружу значения
    int purge(time_t);
    // Установить первоначальное значения параметра, найти и установить
    // идентификатор этой новой записи
    int setup_parameter(sa_parameter_info_t&);
    // Изменить/вернуть ежим журналирования БД для ускорения занесения в неё данных
    void accelerate(bool);
    // Обновить отметку времени последнего обмена с системой сбора в таблице InternalSMAD
    int update_last_acq_info();

  private:
    DISALLOW_COPY_AND_ASSIGN(InternalSMAD);
    // Создать таблицу по заданному имени и SQL-выражению
    bool create_table(const char*, const char*);
    // Удалить целиком указанную таблицу 
    bool drop_table(const char*);
    // Удалить из таблиц аккумулятора и НСИ сведения об указанной СС
    int drop_dict(const char*, uint64_t);
    // Проверка существования указанной таблицы
    bool table_existance(const char*);
    // Получить идентификатор системы сбора с указанным именем
    bool get_sa_reference(char*, uint64_t&);

    // -------------------------------------------------------------------
    // Название таблицы с описанием систем сбора, включенных в эту зону InternalSMAD
    static const char *s_SMAD_DESCRIPTION_TABLENAME;
    // Название таблицы с описанием параметров, принадлежащих данной СС
    static const char *s_SA_DICT_TABLENAME;
    // Название таблицы, аккумулирующей данные от всех систем сбора этой зоны InternalSMAD
    static const char *s_SA_DATA_TABLENAME;

    // Проверка существования таблицы
    static const char *s_SQL_CHECK_TABLE_TEMPLATE;
    // Команда создания таблицы InternalSMAD в случае, если ранее её не было
    static const char *s_SQL_CREATE_SMAD_TEMPLATE;
    // Шаблон занесения данных в таблицу InternalSMAD
    static const char *s_SQL_INSERT_SMAD_TEMPLATE;
    // Команда создания таблицы НСИ СС в случае, если ранее её не было
    static const char *s_SQL_CREATE_SA_TEMPLATE;
    // Шаблон заголовка SQL-выражения занесения данных целочисленного типа в базу
    static const char *s_SQL_INSERT_SA_HEADER_INT_TEMPLATE;
    // Шаблон заголовка SQL-выражения занесения данных с плав. запятой в базу
    static const char *s_SQL_INSERT_SA_HEADER_FLOAT_TEMPLATE;
    // Шаблон тела SQL-выражения занесения данных целочисленного типа в базу
    static const char *s_SQL_INSERT_SA_VALUES_INT_TEMPLATE;
    // Шаблон тела SQL-выражения занесения данных с плав. запятой в базу
    static const char *s_SQL_INSERT_SA_VALUES_FLOAT_TEMPLATE;
    // Таблица с накапливаемыми значениями параметров
    static const char *s_SQL_CREATE_SA_DATA_TEMPLATE;
    // Шаблон заголовка SQL-выражения занесения целочисленных данных в базу
    static const char *s_SQL_INSERT_SA_DATA_HEADER_INT_TEMPLATE;
    // Шаблон тела SQL-выражения занесения целочисленных данных в базу
    static const char *s_SQL_INSERT_SA_DATA_VALUES_INT_TEMPLATE;
    // Шаблон заголовка SQL-выражения занесения данных с плав. запятой в базу
    static const char *s_SQL_INSERT_SA_DATA_HEADER_FLOAT_TEMPLATE;
    // Шаблон тела SQL-выражения занесения данных с плав. запятой в базу
    static const char *s_SQL_INSERT_SA_DATA_VALUES_FLOAT_TEMPLATE;
    // Выбрать ссылку параметра из справочной таблицы указанной системы сбора
    static const char *s_SQL_SELECT_SA_DICT_REF;
    // Выбрать количество записей об указанной системе сбора из конфигурации InternalSMAD
    static const char *s_SQL_SELECT_SMAD_SA_COUNT;
    // Прочитать идентификатор искомой СС из таблицы InternalSMAD
    static const char *s_SQL_SELECT_SA_ID_FROM_SMAD;
    // Удалить данные очищаемой СС из аккумуляторной таблицы по идентификатору
    static const char *s_SQL_DELETE_SA_DATA_BY_REF;
    // Удалить данные очищаемой СС из аккумуляторной таблицы по её имени
    static const char *s_SQL_DELETE_SA_DATA_BY_NAME;
    // Удалить НСИ очищаемой СС по её идентификатору
    static const char *s_SQL_DELETE_SA_DICT_BY_REF;
    // Удалить НСИ очищаемой СС по её имени
    static const char *s_SQL_DELETE_SA_DICT_BY_NAME;
    // Обновить время последнего успешного обмена с указанной СС
    static const char *s_SQL_UPDATE_SMAD_SA_LASTPROBE;

    sqlite3* m_db;
    smad_connection_state_t  m_state;
    gof_t_SacNature    m_sa_nature;
    char*    m_db_err;
    char*    m_snapshot_filename;
    char*    m_sa_name;
    uint64_t m_sa_reference;
};

#endif

