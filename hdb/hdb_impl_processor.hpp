#ifndef HIST_IMPL_PROCESSOR_HPP
#define HIST_IMPL_PROCESSOR_HPP
#pragma once

#include "sqlite3.h"

#if defined HAVE_CONFIG_H
#include "config.h"
#endif

#include "xdb_common.hpp"

#define within(num) (int) ((float) (num) * random () / (RAND_MAX + 1.0))

// Тип генератора предыстории
typedef enum {
  STAGE_NONE    = 0,
  PER_1_MINUTE  = 1,
  PER_5_MINUTES = 2,
  PER_HOUR      = 3,
  PER_DAY       = 4,
  PER_MONTH     = 5,
  STAGE_LAST    = PER_MONTH + 1
} sampler_type_t; // HistoryType из rtap_db.h

// Элементарная запись связки идентификатора Тега и Глубины хранения истории для него
typedef struct {
  int tag_id;  		// идентификатор тега
  int history_type;	// depth, глубина хранения истории
} history_info_t;

// Элементарная запись единичного семпла
typedef struct {
  time_t datehourm;
  double val;
  char valid;
  history_info_t info;
} historized_attributes_t;

// Связка между собирателями текущего и следующего типов
typedef struct {
  sampler_type_t current;
  const char* pair_name_current;
  sampler_type_t next;
  const char* pair_name_next;
  // Количество анализируемых семплов предыдущей стадии для
  // получения одного семпла текущей стадии
  int num_prev_samples;
  // Суффикс файла, содержащего предысторию данного цикла
  char suffix[3];
  // Длительность интервала данного типа в секундах
  // минутная = 60 секунд
  // 5 минут = 300 секунд
  // 1 час = 3600 секунд
  // 1 сутки = 86400 секунд
  int duration;
} state_pair_t;

typedef std::map <std::string, history_info_t> map_history_info_t;
// Тип для передачи нужных данных по одному элементарному семплу
typedef std::map <const std::string, historized_attributes_t> HistorizedInfoMap_t;


class Historian {
  public:
    Historian(const char*, const char*);
   ~Historian();

    // Подключиться к БДРВ и HDB
    bool Connect();
    // Отключиться от БДРВ и HDB
    bool Disconnect();
    // Выполнить рассчёт и сохранение среза данных, в зависимости от указанного времени
    bool Make(time_t);
    // Загрузить набор семплов для данного тега, указанной глубины, начиная с указанной даты
    bool load_samples_period_per_tag(const char*, const sampler_type_t, time_t, const int);

  private:
    // Шаблон SQL-выражения создания таблицы НСИ в HDB
    static const char *s_SQL_CREATE_TABLE_DICT_TEMPLATE;
    // Шаблон SQL-выражения создания таблицы с данными в HDB
    static const char *s_SQL_CREATE_TABLE_DATA_TEMPLATE;
    static const char *s_SQL_CREATE_INDEX_DATA_TEMPLATE;
    static const char *s_SQL_CHECK_TABLE_TEMPLATE;
    static const char *s_SQL_INSERT_DICT_HEADER_TEMPLATE;
    static const char *s_SQL_INSERT_DICT_VALUES_TEMPLATE;
    static const char *s_SQL_SELECT_DICT_TEMPLATE;
    static const char *s_SQL_DELETE_DICT_TEMPLATE;
    static const char *s_SQL_INSERT_DATA_ONE_ITEM_HEADER_TEMPLATE;
    static const char *s_SQL_INSERT_DATA_ONE_ITEM_VALUES_TEMPLATE;
    static const char *s_SQL_READ_DATA_ITEM_TEMPLATE;
    static const char *s_SQL_READ_DATA_AVERAGE_ITEM_TEMPLATE;

    xdb::RtDatabase* m_rtdb;
    xdb::RtConnection* m_rtdb_conn;
    sqlite3     *m_hist_db;      // Указатель на объект БД Истории
    char        *m_hist_err;     // Сведения об ошибке при работе с БД Истории
    bool         m_history_db_connected;
    bool         m_rtdb_connected;
    // Название файла со снимком HDB
    char m_history_db_filename[SERVICE_NAME_MAXLEN + 1];
    // Название Службы БДРВ, для получения оттуда данных
    char m_rtdb_name[SERVICE_NAME_MAXLEN + 1];

    // Результат выполнения запроса на список точек с предысторией в формате RTDB.
    // Требует конвертации, чтобы хранить еще и идентификатор записи в HDB.
    xdb::map_name_id_t m_raw_actual_rtdb_points;
    // Текущие теги из БДРВ
    map_history_info_t m_actual_rtdb_points;
    // Теги из существующего снимка Исторической БД
    map_history_info_t m_actual_hist_points;
    // Разница между старым и новым наборами тегов Исторической БД
    map_history_info_t m_need_to_create_hist_points;
    map_history_info_t m_need_to_delete_hist_points;
    historized_attributes_t m_historized[MAX_PORTION_SIZE_LOADED_HISTORY];


    bool history_db_connect();
    bool rtdb_connect();
    bool rtdb_disconnect();
    bool history_db_disconnect();

    bool is_table_exist(sqlite3*, const char*);
    // Создать индексы, если они ранее не существовали
    bool createIndexes();
    // Создать соответствующую таблицу, если она ранее не существовала
    bool createTable(const char*);

    bool make_history_samples_by_type(const sampler_type_t, time_t);
    bool load_samples_list_by_type(const sampler_type_t, time_t, xdb::map_name_id_t, HistorizedInfoMap_t&);
    bool tune_dictionaries();

    bool rtdb_get_info_historized_points();
#if (HIST_DATABASE_LOCALIZATION==LOCAL)
    bool local_rtdb_get_info_historized_points(xdb::map_name_id_t&);
#endif
#if (HIST_DATABASE_LOCALIZATION==DISTANT)
    bool remote_rtdb_get_info_historized_points(xdb::map_name_id_t&);
#endif
    int getPointsFromDictHDB(map_history_info_t&);
    int deletePointsDictHDB(map_history_info_t&);
    // Удалить данные из таблицы HIST с заданными идентификаторами
    int deletePointsDataHDB(map_history_info_t&);
    int createPointsDictHDB(map_history_info_t&);
    int calcDifferences(map_history_info_t&, map_history_info_t&, map_history_info_t&);
    int getPointIdFromDictHDB(const char*);
    void accelerate(bool);

    // Занести все историзированные значения из HistorizedInfoMap_t в HDB
    int store_history_samples(HistorizedInfoMap_t&, sampler_type_t);
    // Выбрать среднее значение VAL из семплов предыдущего типа
    bool select_avg_values_from_sample(historized_attributes_t&, const sampler_type_t);
    time_t roundTimeToNextEdge(time_t, const sampler_type_t, bool);
    int readValuesFromDataHDB(int,       // Id точки
                          sampler_type_t,// Тип читаемой истории
                          time_t,        // Начало диапазона читаемой истории
                          time_t,        // Конец диапазона читаемой истории
                          historized_attributes_t*); // Прочитанные данные семплов

};

#endif

