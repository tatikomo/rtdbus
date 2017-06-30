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
#define SECTION_NAME_DESCRIPTION    "DESCRIPTION"   // Блок описания словаря
#define SECTION_NAME_SITE       "SITE"          // Код сайта, с которым осуществляется обмен
#define SECTION_NAME_FLOW       "FLOW"          // Направление переноса данных
#define SECTION_FLOW_VALUE_ACQ  "ACQ"           // перенос данных с Сайта
#define SECTION_FLOW_VALUE_DIFF "DIFF"          // перенос данных на Сайт
#define SECTION_NAME_ACQINFOS   "ACQINFOS"      // Название раздела с данными словаря
#define SECTION_NAME_TAG        "TAG"           // Тег параметра
#define SECTION_NAME_TYPE       "TYPE"          // Инфотип
#define SECTION_NAME_LED        "LED"           // Идентификатор обмена
#define SECTION_NAME_CATEGORY   "CATEGORY"      // Категория телеизмерения
#define SECTION_NAME_ASSOCIATES "ASSOCIATES"    // Название раздела ассоциированных с Параметром структур ELEMSTRUCT
#define SECTION_NAME_STRUCT     "STRUCT"        // Экземпляр ассоциированной с Параметром структуры

/*
<?xml version="1.0" encoding="utf-8" ?>
<!-- SQL XML created by WWW SQL Designer, https://github.com/ondras/wwwsqldesigner/ -->
<!-- Active URL: http://ondras.zarovi.cz/sql/demo/ -->
<sql>
<datatypes db="sqlite">
	<group label="Affinity">
		<type label="Text" default="" length="1" sql="TEXT" quote="'" color="rgb(255,200,200)"/>
		<type label="Numeric" default="0" length="0" sql="NUMERIC" quote="" color="rgb(238,238,170)"/>
		<type label="Integer" default="0" length="0" sql="INTEGER" quote="" color="rgb(238,238,170)"/>
		<type label="Real" default="0" length="0" sql="REAL" quote="" color="rgb(238,238,170)"/>
		<type label="None" default="" length="0" sql="NONE" quote="" color="rgb(200,255,200)"/>
	</group>
</datatypes><table x="489" y="329" name="ELEMSTRUCT">
<row name="STRUCT_ID" null="0" autoincrement="1">
<datatype>INTEGER</datatype>
<default>NOT NULL</default></row>
<row name="ASSOCIATE" null="0" autoincrement="0">
<datatype>TEXT(6)</datatype>
<default>'NULL'</default></row>
<row name="NAME" null="0" autoincrement="0">
<datatype>TEXT(6)</datatype>
<default>'NULL'</default></row>
<row name="CLASS" null="0" autoincrement="0">
<datatype>INTEGER</datatype>
</row>
<key type="PRIMARY" name="">
<part>STRUCT_ID</part>
</key>
<comment>Таблица НСИ. Содержит соответствие между названием Структуры и ее Ассоциацией.</comment>
</table>
<table x="274" y="410" name="FIELDS">
<row name="FIELD_ID" null="0" autoincrement="1">
<datatype>INTEGER</datatype>
</row>
<row name="ELEMSTRUCT_REF" null="0" autoincrement="0">
<datatype>INTEGER</datatype>
<relation table="ELEMSTRUCT" row="STRUCT_ID" />
</row>
<row name="ELEMTYPE_REF" null="0" autoincrement="0">
<datatype>INTEGER</datatype>
<relation table="ELEMTYPE" row="TYPE_ID" />
</row>
<row name="ATTR" null="1" autoincrement="0">
<datatype>TEXT(32)</datatype>
<default>NULL</default></row>
<row name="TYPE" null="1" autoincrement="0">
<datatype>INTEGER</datatype>
<default>NULL</default></row>
<row name="LENGTH" null="1" autoincrement="0">
<datatype>INTEGER</datatype>
<default>NULL</default></row>
<row name="IDX" null="0" autoincrement="0">
<datatype>INTEGER</datatype>
<default>NUL</default></row>
<key type="PRIMARY" name="">
<part>FIELD_ID</part>
</key>
<comment>Таблица НСИ. Содержит описание полей и атрибутов БДРВ каждой Структуры.</comment>
</table>
<table x="747" y="184" name="DATA">
<row name="DATA_ID" null="0" autoincrement="1">
<datatype>INTEGER</datatype>
</row>
<row name="TAG" null="0" autoincrement="0">
<datatype>TEXT(32)</datatype>
<default>'NULL'</default></row>
<row name="LED" null="1" autoincrement="0">
<datatype>INTEGER</datatype>
<default>NULL</default></row>
<row name="OBJCLASS" null="0" autoincrement="0">
<datatype>INTEGER</datatype>
</row>
<row name="CATEGORY" null="0" autoincrement="0">
<datatype>INTEGER</datatype>
</row>
<row name="ALARM_INDOC" null="0" autoincrement="0">
<datatype>INTEGER</datatype>
</row>
<row name="HISTO_INDIC" null="0" autoincrement="0">
<datatype>INTEGER</datatype>
</row>
<key type="PRIMARY" name="">
<part>DATA_ID</part>
</key>
<comment>Оперативная таблица. Содержит описания всех Параметров, заполняется путем загрузки словарей обмена.</comment>
</table>
<table x="250" y="12" name="PROCESSING">
<row name="PROCESS_ID" null="0" autoincrement="1">
<datatype>INTEGER</datatype>
<default>NULL</default></row>
<row name="SITE_REF" null="0" autoincrement="0">
<datatype>INTEGER</datatype>
<default>NOT NULL</default><relation table="SITES" row="SITE_ID" />
</row>
<row name="DATA_REF" null="0" autoincrement="0">
<datatype>INTEGER</datatype>
<default>NOT NULL</default><relation table="DATA" row="DATA_ID" />
</row>
<row name="TYPE" null="0" autoincrement="0">
<datatype>INTEGER</datatype>
<default>NULL</default></row>
<row name="LAST_UPDATE" null="1" autoincrement="0">
<datatype>INTEGER</datatype>
<default>NULL</default></row>
<key type="PRIMARY" name="">
<part>PROCESS_ID</part>
</key>
<comment>Оперативная таблица. Содержит связь между Параметром и используемым его Сайтом, с типом связи - прием от Сайта или передача в адрес Сайта.</comment>
</table>
<table x="37" y="152" name="ASSOCIATES_LINK">
<row name="DATA_REF" null="0" autoincrement="0">
<datatype>INTEGER</datatype>
<default>NOT NULL</default><relation table="DATA" row="DATA_ID" />
</row>
<row name="ELEMSTRUCT_REF" null="0" autoincrement="0">
<datatype>INTEGER</datatype>
<default>NOT NULL</default><relation table="ELEMSTRUCT" row="STRUCT_ID" />
</row>
<key type="PRIMARY" name="">
<part>DATA_REF</part>
</key>
<comment>Оперативная таблица. Содержит соответствие между Тегом и используемыми им Структурами. Заполняется при загрузке словарей обмена</comment>
</table>
<table x="41" y="310" name="FIELDS_LINK">
<row name="LINK_ID" null="0" autoincrement="1">
<datatype>INTEGER</datatype>
</row>
<row name="DATA_REF" null="0" autoincrement="0">
<datatype>INTEGER</datatype>
<relation table="DATA" row="DATA_ID" />
</row>
<row name="ELEMSTRUCT_REF" null="0" autoincrement="0">
<datatype>INTEGER</datatype>
<relation table="ELEMSTRUCT" row="STRUCT_ID" />
</row>
<row name="FIELDS_REF" null="0" autoincrement="0">
<datatype>INTEGER</datatype>
<relation table="FIELDS" row="FIELD_ID" />
</row>
<row name="LAST_UPDATE" null="0" autoincrement="0">
<datatype>INTEGER</datatype>
<default>NULL</default></row>
<row name="VAL_INT" null="1" autoincrement="0">
<datatype>INTEGER</datatype>
<default>NULL</default></row>
<row name="VAL_DOUBLE" null="1" autoincrement="0">
<datatype>REAL</datatype>
<default>NULL</default></row>
<row name="VAL_TEXT" null="1" autoincrement="0">
<datatype>TEXT</datatype>
<default>NULL</default></row>
<key type="PRIMARY" name="">
<part>LINK_ID</part>
</key>
<comment>Оперативная таблица. Содержит связь между Структурой данных, набором входящих в неё полей, и их оперативными значениями </comment>
</table>
<table x="601" y="32" name="SITES">
<row name="SITE_ID" null="0" autoincrement="1">
<datatype>INTEGER</datatype>
</row>
<row name="NAME" null="1" autoincrement="0">
<datatype>TEXT(32)</datatype>
<default>NULL</default></row>
<row name="STATE" null="1" autoincrement="0">
<datatype>INTEGER</datatype>
<default>NULL</default></row>
<row name="NATURE" null="0" autoincrement="0">
<datatype>INTEGER</datatype>
</row>
<row name="LAST_UPDATE" null="1" autoincrement="0">
<datatype>INTEGER</datatype>
<default>NULL</default></row>
<key type="PRIMARY" name="">
<part>SITE_ID</part>
</key>
<comment>Оперативная таблица. Заполняется из конфига egsa и содержит известные локально Сайты.</comment>
</table>
<table x="669" y="449" name="ELEMTYPE">
<row name="TYPE_ID" null="0" autoincrement="1">
<datatype>INTEGER</datatype>
<default>NULL</default></row>
<row name="NAME" null="1" autoincrement="0">
<datatype>TEXT(6)</datatype>
<default>NULL</default></row>
<row name="TYPE" null="1" autoincrement="0">
<datatype>INTEGER</datatype>
<default>NULL</default></row>
<row name="SIZE" null="1" autoincrement="0">
<datatype>TEXT(6)</datatype>
<default>NULL</default></row>
<key type="PRIMARY" name="">
<part>TYPE_ID</part>
</key>
<comment>Таблица НСИ. Содержит описание элементарного типа - название, тип данных, длина строки.</comment>
</table>
</sql>

 */
 
// NB: Поскольку один и тот же локальный параметр БДРВ может передаваться нескольким получателям, нужно ввести связь с несколькими СС для таблицы DATA.
// Для этого используется таблица PROCESSING, позволяющая привязать один Параметр к разным видам обработки разных СС.
//
// Пример для ЛПУ:
// Локальный параметр "Состояние Цеха" передается в ГТП и в несколько смежных ЛПУ. Нужно знать, в какое объект отправлять этот Параметр, и состояние передачи.
// Для этого Параметра в PROCESSING будет несколько записей (количество = ГТП + смежные ЛПУ) с DATA_REF, равным DATA_ID в таблице DATA, соответствующими SITE_REF
// и атрибутом времени передачи.
//
// Структура общей таблицы "SITES"
// ---+-------------+-----------+--------------+
//  # |  Имя поля   | Тип поля  | По умолчанию |
// ---+-------------+-----------+--------------+
//  1 | SITE_ID     | INTEGER   | PK:generated |<---+   // Идентификатор записи
//  2 | NAME        | TEXT      |              |    |   // Код СС
//  3 | STATE       | INTEGER   | 0            |    |   // Состояние
//  4 | NATURE      | INTEGER   | 0            |    |   // Тип СС - ЛПУ, ЦДП, ЭЖД, ...
//  5 | LAST_UPDATE | INTEGER   | 0            |    |   // Время с последнего взаимодействия
// ---+-------------+-----------+--------------+    |
//                                                  |
// Структура таблицы PROCESSING Параметров, подлежащих отправки в соответствующие внешние системы (заполняется при загрузке словаря ACQ|SEND)
// ---+-------------+-----------+--------------+    |
//  # |  Имя поля   | Тип поля  | По умолчанию |    |
// ---+-------------+-----------+--------------+    |
//  1 | PROCESS_ID  | INTEGER   | PK:generated |    |
//  2 | SITE_REF    | INTEGER   | FK: NOT NULL +----+
//  3 | DATA_REF    | INTEGER   | FK: NOT NULL +----+
//  4 | PROC_TYPE   | INTEGER   |              |    |   // Тип обработки Параметра: Прием (ACQ)/передача (DIFF)/...
//  5 | LAST_PROC   | INTEGER   | 0            |    |   // Время с последнего взаимодействия
// ---+-------------+-----------+--------------+    |
//                                                  |
// Структура таблицы текущих оперативных данных СС "DATA"
// TODO: возможно, не стоит хранить значения Параметров в SMED, сразу отправляя их в БДРВ
// ---+-------------+-----------+--------------+    |
//  # |  Имя поля   | Тип поля  | По умолчанию |    |
// ---+-------------+-----------+--------------+    v
//  1 | DATA_ID     | INTEGER   | PK:generated |<---+<+
//  2 | TAG         | CHAR[32]  | NOT NULL     |    ^ |  // Тег
//  3 | LED         | INTEGER   | NOT NULL     |    | |  // Идентификатор параметра из обменного файла
//  4 | OBJCLASS    | INTEGER   | 127          |    | |  // Инфотип параметра из обменного файла
//  5 | CATEGORY    | INTEGER   | NOT NULL     |    | |  // Категория параметра (первичный 'P', вторичный 'S', или третичный 'T' цикл сбора данных)
//  6 | ALARM_INDIC | INTEGER   | 0            |    | |  // Признак наличия тревоги по данному параметру
//  7 | HISTO_INDIC | INTEGER   | 0            |    | |  // Период предыстории (0 = NONE, QH, HOUR, DAY, ...)
// ---+-------------+-----------+--------------+    | |
//                                                  | |
// Таблица "ASSOCIATES_LINK" для связи "DATA" и "ELEMSTRUCTS"
// ---+-------------+-----------+--------------+    | |
//  # |  Имя поля   | Тип поля  | По умолчанию |    | |
// ---+-------------+-----------+--------------+    | |
//  1 | DATA_REF    | INTEGER   | FK: NOT NULL |----+ |  //  Ссылка на элемент таблицы "DATA" параметра
//  2 | ELEMSTR_REF | INTEGER   | FK: NOT NULL |----+ | //  Ссылка на таблицу с описанием Композитной структуры параметра
// ---+-------------+-----------+--------------+    | |
//                                                  | |
// Таблица с данными ELEMSTRUCT                     | |
// ---+-------------+-----------+--------------+    | |
//  # |  Имя поля   | Тип поля  | По умолчанию |    | |
// ---+-------------+-----------+--------------+    v |
//  1 | STRUCT_ID   | INTEGER   | PK:generated |<---+ |
//  2 | NAME        | CHAR[6]   |              |  ^ ^ | // Название Композитной структуры
//  3 | ASSOCIATE   | CHAR[6]   |              |  | | | // Название Ассоциации
//  4 | CLASS       | INTEGER   |              |  | | | // Класс Композитной структуры
// ---+-------------+-----------+--------------+  | | |
//                                                | | |
// Таблица FIELDS_LINK связи полей Параметра и их родительской Структуры
// ---+-------------+-----------+--------------+  | | |
//  # |  Имя поля   | Тип поля  | По умолчанию |  | | |
// ---+-------------+-----------+--------------+  | | |
//  1 | LINK_ID     | INTEGER   | PK:generated |  | | |
//  2 | STRUCT_REF  | INTEGER   | FK: NOT NULL +--+ | |
//  3 | FIELD_REF   | INTEGER   | FK: NOT NULL +--+ | |
//  4 | DATA_REF    | INTEGER   | FK: NOT NULL +------+  // Коэффициент предобразования значения параметра перед передачей его значения в БДРВ
//  5 | LAST_UPDATE | INTEFER   | 0            |  | |    // Время последнего занесения значения данного параметра в SMED
//  6 | VAL_INT     | INTEGER   |              |  | |    // Значение атрибута типов "Целое"/"Логическое"/"Время" (время в SQLite хранится в INTEGER)
//  7 | VAL_DOUBLE  | DOUBLE    |              |  | |    // Значение атрибута типа "С плав. точкой"
//  8 | VAL_STRING  | VARCHAR   | NULL         |  | |    // Значение атрибута типа "Строка" (TODO: Можно сделать необязательной?)
// ---+-------------+-----------+--------------+  | |
//                                                | |
// Таблица FIELDS набора словарных полей          | |
// ---+-------------+-----------+--------------+  | |
//  # |  Имя поля   | Тип поля  | По умолчанию |  | |
// ---+-------------+-----------+--------------+  | |
//  1 | FIELD_ID    | INTEGER   | generated    |<-+ |
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
//  1 | TYPE_ID     | INTEGER   | PK:generated |<---+
//  2 | NAME        | CHAR[6]   | NOT NULL     |        // Название элемента (пример: "L1800")
//  3 | TYPE        | INTEGER   |              |        // Тип данных элемента (числовой код типов Real, Int, String, Time, Logic)
//  4 | SIZE        | CHAR[6]   |              |        // Формат размерности элемента (к примеру "11e4", "2.2", "U20", "8", ...)
// ---+-------------+-----------+--------------+
//
// ==========================================================================================================
// структура записи SMED для Телеметрии
typedef struct {
  gof_t_UniversalName s_Name;       // name (TAG) of the exchanged data
  int   i_LED;                      // identifier of the exchanged data
  int   h_RecId;                    // SMED record identifier (DATA_ID из SMED)
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
    void accelerate(bool);
    // Найти и вернуть информацию по параметру с идентификатором LED для указанного Сайта
    int get_diff_info(const gof_t_UniversalName, const int, esg_esg_odm_t_ExchInfoElem*);
    int get_acq_info(const gof_t_UniversalName, const int, esg_esg_odm_t_ExchInfoElem*);
    int load_dict(const char*);
    int processing(const gof_t_UniversalName s_IAcqSiteId,
                   const esg_esg_edi_t_StrComposedData* pr_IInternalCData,
                   elemstruct_item_t* pr_ISubTypeElem,
                   const esg_esg_edi_t_StrQualifyComposedData* pr_IQuaCData,
                   const struct timeval d_IReceivedDate);
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

    // Словарь для хранения связи между строкой и идентификатором
    typedef std::map <const std::string, size_t, map_cmp_str> map_id_by_name_t;

    // Первоначальное создание таблиц и индексов SMED
    int create_internal();
    // Заполнение данными НСИ
    int load_internal(elemtype_item_t*, elemstruct_item_t*);
    // Первоначальное заполнение таблицы DATA описателями Параметров (TAG,LED,OBJCLASS,...)
    int load_data_dict(const char*, std::vector<exchange_parameter_t*>&, sa_flow_direction_t);
    int get_map_id(const char*, map_id_by_name_t&);
    int get_info(const gof_t_UniversalName, sa_flow_direction_t, const int, esg_esg_odm_t_ExchInfoElem*);

    sqlite3* m_db;
    smad_connection_state_t  m_state;
    char*    m_db_err;
    EgsaConfig* m_egsa_config;  // Конфигурация нужна для восстановления структуры таблиц и загрузки начальных значений
    char*    m_snapshot_filename;
    map_id_by_name_t m_elemstruct_dict; // Связи между названием Композитной структуры и её идентификатором в БД
    map_id_by_name_t m_elemstruct_associate_dict; // Связи между ассоциацией Композитной структуры и её идентификатором в БД
    map_id_by_name_t m_sites_dict;      // Связи между названием Сайта и его идентификатором в БД
};

#endif

