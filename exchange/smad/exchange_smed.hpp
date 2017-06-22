#if !defined EXCHANGE_SMED_HPP
#define EXCHANGE_SMED_HPP
#pragma once

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

// Общесистемные заголовочные файлы
#include <assert.h>
#include <time.h>
#include <vector>

// Служебные заголовочные файлы сторонних утилит
class sqlite3;

// Служебные файлы RTDBUS
#include "exchange_config.hpp"

// Название разделов к словаре обмена данными (ESG_EXACQINFOS / ESG_EXSNDINFOS)
#define SECTION_NAME_ACQINFOS   "ACQINFOS"
#define SECTION_NAME_TAG        "TAG"
#define SECTION_NAME_TYPE       "TYPE"
#define SECTION_NAME_LED        "LED"
#define SECTION_NAME_CATEGORY   "CATEGORY"
#define SECTION_NAME_ASSOCIATES "ASSOCIATES"    // Название раздела ассоциированных с Параметром структур ELEMSTRUCT
#define SECTION_NAME_STRUCT     "STRUCT"        // Экземпляр ассоциированной с Параметром структуры

// Структура таблицы текущих оперативных данных СС "DATA"
// ---+-------------+-----------+--------------+
//  # |  Имя поля   | Тип поля  | По умолчанию |
// ---+-------------+-----------+--------------+
//  1 | DATA_ID     | INTEGER   | generated    |<---+
//  2 | TAG         | CHAR[32]  | NOT NULL     |    |   // Тег
//  3 | LED         | INTEGER   | NOT NULL     |    |   // Идентификатор параметра из обменного файла
//  4 | OBJCLASS    | INTEGER   | 127          |    |   // Инфотип параметра из обменного файла
//  5 | CATEGORY    | INTEGER   | NOT NULL     |    |   // Категория параметра (первичный 'P', вторичный 'S', или третичный 'T' цикл сбора данных)
//  6 | ALARM_INDIC | INTEGER   | 0            |    |   // Признак наличия тревоги по данному параметру
//  7 | HISTO_INDIC | INTEGER   | 0            |    |   // Период предыстории (0 = NONE, QH, HOUR, DAY, ...)
//  8 | CONV_COEFF  | FLOAT     | 1.0          |    |   // Коэффициент предобразования значения параметра перед передачей его значения в БДРВ
//  9 | LAST_UPDATE | TIME      |              |    |   // Время последнего занесения значения данного параметра в SMED
// 10 | VAL_INT     | INTEGER   |              |    |   // Значение атрибута типа "Целое" или "Логическое"
// 11 | VAL_TIME    | TIME      |              |    |   // Значение атрибута типа "Время"
// 12 | VAL_DOUBLE  | DOUBLE    |              |    |   // Значение атрибута типа "С плав. точкой"
// 13 | VAL_STRING  | VARCHAR   | NULL         |    |   // Значение атрибута типа "Строка" (TODO: Можно сделать необязательной?)
// ---+-------------+-----------+--------------+    |
//                                                  |
// Таблица "ASSOCIATES_LINK" для связи "DATA" и "ELEMSTRUCTS"
// ---+-------------+-----------+--------------+    |
//  # |  Имя поля   | Тип поля  | По умолчанию |    |
// ---+-------------+-----------+--------------+    |
//  1 | DATA_REF    | INTEGER   | FK: NOT NULL |----+   //  Ссылка на элемент таблицы "DATA" параметра
//  2 | ELEMSTR_REF | INTEGER   | FK: NOT NULL |----+   //  Ссылка на таблицу с описанием Композитной структуры параметра
// ---+-------------+-----------+--------------+    |
//                                                  |
// Таблица с данными ELEMSTRUCTS                    |
// ---+-------------+-----------+--------------+    |
//  # |  Имя поля   | Тип поля  | По умолчанию |    |
// ---+-------------+-----------+--------------+    v
//  1 | STRUCT_ID   | INTEGER   | generated    |<---+
//  2 | NAME        | CHAR[6]   |              |    ^   // Название Композитной структуры
//  3 | ASSOCIATE   | CHAR[6]   |              |    |   // Название Ассоциации
//  4 | CLASS       | INTEGER   |              |    |   // Класс Композитной структуры
// ---+-------------+-----------+--------------+    |
//                                                  |
// Таблица FIELDS набора полей                      |
// ---+-------------+-----------+--------------+    |
//  # |  Имя поля   | Тип поля  | По умолчанию |    |
// ---+-------------+-----------+--------------+    |
//  1 | FIELD_ID    | INTEGER   | generated    |    |
//  2 | STRUCT_REF  | INTEGER   | FK: NOT NULL |----+   // Ссылка на родительский элемент Композитной структуры
//  3 | ATTR        | CHAR[21]  |              |        // Название в БДРВ атрибута параметра
//  4 | TYPE        | INTEGER   | 0            |        // Тип в БДРВ атрибута параметра
//  5 | LENGTH      | INTEGER   | 0            |        // Длина (имеет смысл только для строкового типа)
//  6 | ELEMTYP_REF | INTEGER   | FK: NOT NULL |----+   // Ссылка на используемый Базовый элемент Композитной структуры
// ---+-------------+-----------+--------------+    |
//                                                  |
// Таблица Базовых элементов ELEMTYPE               |
// ---+-------------+-----------+--------------+    |
//  # |  Имя поля   | Тип поля  | По умолчанию |    |
// ---+-------------+-----------+--------------+    |
//  1 | TYPE_ID     | INTEGER   | generated    |<---+
//  2 | NAME        | CHAR[6]   | NOT NULL     |        // Название элемента (пример: "L1800")
//  3 | TYPE        | INTEGER   |              |        // Тип данных элемента (числовой код типов Real, Int, String, Time, Logic)
//  4 | SIZE        | CHAR[6]   |              |        // Формат размерности элемента (к примеру "11e4", "2.2", "U20", "8", ...)
// ---+-------------+-----------+--------------+
//
// Структура общей таблицы "SITES"
// ---+-------------+-----------+--------------+
//  # |  Имя поля   | Тип поля  | По умолчанию |
// ---+-------------+-----------+--------------+
//  1 | ID          | INTEGER   | generated    |
//  2 | NAME        | TEXT      |              |        // Код СС
//  3 | STATE       | INTEGER   | 0            |        // Состояние
//  4 | NATURE      | INTEGER   | 0            |        // Тип СС - ЛПУ, ЦДП, ЭЖД, ...
//  5 | LAST_UPDATE | TIME      |              |        // Время с последнего взаимодействия
// ---+-------------+-----------+--------------+

// ==========================================================================================================
// структура записи SMED для Телеметрии
typedef struct {
  gof_t_UniversalName s_Name;       // name (TAG) of the exchanged data
  int   i_LED;                      // identifier of the exchanged data
  int   h_RecId;                    // SMED record identifier
  int   infotype;                   // OBJCLASS
  telemetry_category_t o_Categ;     // category (Primary/Secondary/Exploitation)
  bool  b_AlarmIndic;               // Alarm indication
  int   i_HistoIndic;               // Historical indication
  float f_ConvertCoeff;             // Conversion coefficient
  struct timeval d_LastUpdDate;
  // TODO: добавить универсальную структуру хранения значений параметра - union {}
} esg_esg_odm_t_ExchInfoElem;

// ==========================================================================================================
class SMED
{
  public:

    SMED(EgsaConfig*, const char*);
   ~SMED();

    smad_connection_state_t state() { return m_state; };
    smad_connection_state_t connect();
    // Найти и вернуть информацию по параметру с идентификатором LED для указанного Сайта
    int get_info(const gof_t_UniversalName, const int, esg_esg_odm_t_ExchInfoElem*);
    int load_dict(const char*);

  private:
    DISALLOW_COPY_AND_ASSIGN(SMED);

    struct map_cmp_str {
      // Оператор сравнения строк в m_site_map.
      // Иначе происходит арифметическое сравнение значений указателей, но не содержимого строк
      bool operator()(const std::string& a, const std::string& b) { return a.compare(b) < 0; }
    };

    // Набор данных Сайта из соответствующего словаря
    typedef struct {
      // Поля TAG, TYPE, LED, CATEGORY
      esg_esg_odm_t_ExchInfoElem info;
      // Набор названий вложенных используемых этим Параметром STRUCTS
      std::vector<std::string> struct_list;
    } exchange_parameter_t;

    typedef std::map <const std::string, size_t, map_cmp_str> map_id_by_name_t;

    // Первоначальное создание таблиц и индексов SMED
    int create_internal();
    // Заполнение данными НСИ
    int load_internal(elemtype_item_t*, elemstruct_item_t*);
    int store_data(std::vector<exchange_parameter_t*>&);
    int get_map_id(const char*, map_id_by_name_t&);

    sqlite3* m_db;
    smad_connection_state_t  m_state;
    char*    m_db_err;
    EgsaConfig* m_egsa_config;  // Конфигурация нужна для восстановления структуры таблиц и загрузки начальных значений
    char*    m_snapshot_filename;
    map_id_by_name_t m_elemstruct_dict;  // Словарь для хранения связи между строкой и идентификатором
};

#endif

