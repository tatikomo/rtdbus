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
#include "glog/logging.h"

class sqlite3;
class AcqSiteEntry;
class Request;
class Cycle;
class SMED;

// Служебные файлы RTDBUS
#include "xdb_common.hpp"
#include "exchange_config.hpp"
#include "exchange_config_egsa.hpp"

// Название разделов к словаре обмена данными (ESG_EXACQINFOS / ESG_EXSNDINFOS)
#define SECTION_NAME_DESCRIPTION    "DESCRIPTION"   // Блок описания словаря
#define SECTION_NAME_SITE       "SITE"          // Код сайта, с которым осуществляется обмен
#define SECTION_NAME_FLOW       "FLOW"          // Направление переноса данных
#define SECTION_FLOW_VALUE_ACQ  "ACQUISITION"   // перенос данных с Сайта
#define SECTION_FLOW_VALUE_DIFF "DIFFUSION"     // перенос данных на Сайт
#define SECTION_NAME_ACQINFOS   "ACQINFOS"      // Название раздела с данными словаря
#define SECTION_NAME_TAG        "TAG"           // Тег параметра
#define SECTION_NAME_TYPE       "TYPE"          // Инфотип
#define SECTION_NAME_LED        "LED"           // Идентификатор обмена
#define SECTION_NAME_CATEGORY   "CATEGORY"      // Категория телеизмерения
#define SECTION_NAME_ASSOCIATES "ASSOCIATES"    // Название раздела ассоциированных с Параметром структур ELEMSTRUCT
#define SECTION_NAME_STRUCT     "STRUCT"        // Экземпляр ассоциированной с Параметром структуры

/*
 * Схема расположена в файле test/dat/smed_schema.xml
 * NB: актуализировать файл после изменений
 */

// NB: Поскольку один и тот же локальный параметр БДРВ может передаваться нескольким получателям, нужно ввести связь с несколькими СС для таблицы DATA.
// Для этого используется таблица PROCESSING, позволяющая привязать один Параметр к разным видам обработки разных СС.
//
// Пример для ЛПУ:
// Локальный параметр "Состояние Цеха" передается в ГТП и в несколько смежных ЛПУ. Нужно знать, в какое объект отправлять этот Параметр, и состояние передачи.
// Для этого Параметра в PROCESSING будет несколько записей (количество = ГТП + смежные ЛПУ) с DATA_REF, равным DATA_ID в таблице DATA, соответствующими SITE_REF
// и атрибутом времени передачи.
//
// Структура таблицы "REQUESTS"                         // Таблица Запросов (из конфигурации)
// ---+-------------+-----------+--------------+
//  # |  Имя поля   | Тип поля  | По умолчанию |
// ---+-------------+-----------+--------------+
//  1 | REQ_ID      | INTEGER   | PK:generated |
//  2 | SITE_REF    | INTEGER   | FK: NOT NULL +----+
//  3 | STATE       | INTEGER   | 0            |    |   // Состояние Запроса:  начальное/отложено/отправлено/принято/завершено
//  4 | DATE_START  | INTEGER   | 0            |    |   // Начальное время помещения Запроса в очередь
//  5 | DATE_FINISH | INTEGER   | 0            |    |   // Финальное время существования Запроса в очереди (при превышении - удалить)
//  6 | TYPE        | INTEGER   | NOT_EXIST    |    |   // Тип Запроса: GENCONTROL|URGINFOS|...
//  7 | UID         | INTEGER   | NOT NULL     |    |   // Локально уникальный идентификатор Запроса
//  8 | DATA_NAME   | TEXT      | NULL         |    |   // Название файла с данными этого Запроса (если есть)
//  9 | CYCLE_REF   | INTEGER   | FK: NOT NULL +--+ |   // Ссылка на Цикл, в котором он участвует
// ---+-------------+-----------+--------------+  | |
//                                                | |
// Структура таблицы "CYCLES"                     | |   // Таблица Циклов (из конфигурации)
// ---+-------------+-----------+--------------+  | |
//  # |  Имя поля   | Тип поля  | По умолчанию |  | |
// ---+-------------+-----------+--------------+  | |
//  1 | CYCLE_ID    | INTEGER   | PK:generated |<-+ |
//  2 | NAME        | TEXT      | NOT NULL     |  | |   // Название Цикла
//  3 | FAMILY      | INTEGER   | 0            |  | |   // Тип Цикла: обычный/управление
//  4 | PERIOD      | INTEGER   | 0            |  | |   // Период между событиями Цикла
//  5 | TYPE        | INTEGER   | 0            |  | |   // Идентификатор НСИ Цикла (ID_CYCLE_GENCONTROL..ID_CYCLE_GCT_TGACQ_DIPL)
// ---+-------------+-----------+--------------+  | |
//                                                | |
// Структура таблицы "SITE_CYCLE_LINK"            | |   // Связь между Сайтом и Циклом (из конфигурации)
// ---+-------------+-----------+--------------+  | |
//  # |  Имя поля   | Тип поля  | По умолчанию |  | |
// ---+-------------+-----------+--------------+  | |
//  1 | SA_CYC_ID   | INTEGER   | PK:generated |  | |
//  2 | CYCLE_REF   | INTEGER   | FK: NOT NULL +--+ |   // Ссылка на Цикл
//  3 | SITE_REF    | INTEGER   | FK: NOT NULL +--->+   // Ссылка на Сайт
// ---+-------------+-----------+--------------+    |
//                                                  |
// Структура общей таблицы "SITES"                  |
// ---+-------------+-----------+--------------+    |
//  # |  Имя поля   | Тип поля  | По умолчанию |    |
// ---+-------------+-----------+--------------+    |
//  1 | SITE_ID     | INTEGER   | PK:generated |<---+   // Идентификатор записи
//  2 | NAME        | TEXT      |              |    |   // Код СС
//  3 | SYNTHSTATE  | INTEGER   | 0            |    |   // Состояние: 0=UNREACH / 1=PRE_OPERATIVE / 2=OPERATIVE
//  4 | EXPMODE     | INTEGER   | 1            |    |   // Режим: 1=Работа / 0=Техобслуживание
//  5 | INHIBITION  | INTEGER   | 1            |    |   // Признак запрета: 1=Запрещена / 0=Разрешена
//  6 | NATURE      | INTEGER   | 0            |    |   // Тип СС - ЛПУ, ЦДП, ЭЖД, ...
//  7 | LAST_UPDATE | INTEGER   | 0            |    |   // Время с последнего взаимодействия
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
//  1 | DATA_REF    | INTEGER   | FK: NOT NULL |----+ | //  Ссылка на элемент таблицы "DATA" параметра
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
//  3 | DATA_REF    | INTEGER   | FK: NOT NULL +------+  // Коэффициент предобразования значения параметра перед передачей его значения в БДРВ
//  4 | FIELD_REF   | INTEGER   | FK: NOT NULL +--+ |
//  5 | LAST_UPDATE | INTEFER   | 0            |  | |    // Время последнего занесения значения данного параметра в SMED
//  6 | NEED_SYNC   | INTEFER   | 0            |  | |    // Признак необходимости переноса данных из SMED в БДРВ
//  7 | VAL_INT     | INTEGER   |              |  | |    // Значение атрибута типов "Целое"/"Логическое"/"Время" (время в SQLite хранится в INTEGER)
//  8 | VAL_DOUBLE  | DOUBLE    |              |  | |    // Значение атрибута типа "С плав. точкой"
//  9 | VAL_STR     | VARCHAR   | NULL         |  | |    // Значение атрибута типа "Строка" (TODO: Можно сделать необязательной?)
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


template <typename T>
class SmedObjectList
{
    public:
    SmedObjectList(SMED* _smed) : m_smed(_smed) {
      m_items.clear();
    }
   ~SmedObjectList() {
      clear();
    }
    void insert(T the_new_one) {
      m_items.push_back(the_new_one);
      LOG(INFO) << "SmedObjectList: inserted " << the_new_one->name() << ", size=" << m_items.size();
    }
    // Полностью очистить содержимое
    int clear() {
      LOG(INFO) << "clear ";
      for(T &item : m_items) { delete item; }
      return OK;
    }
    size_t count() const { return m_items.size(); }
    // Обновить содержимое на основе данных из БД
    int refresh();

    // Вернуть элемент по имени Сайта
    T operator[](const char* wtf) {
      for (T &item : m_items) { // access by reference to avoid copying
        if (0 == strcmp(item->name(), wtf)) {
          // Нашли что искали, выходим
          return item;
        }
      }
      return NULL;
    }
    // Вернуть элемент по имени Сайта
    T operator[](const std::string& wtf) {
      for (T &item : m_items) { // access by reference to avoid copying
        if (0 == wtf.compare(item->name())) {
          // Нашли что искали, выходим
          return item;
        }
      }
      return NULL;
    }
    // Вернуть элемент по индексу
    T& operator[](const std::size_t idx) { return m_items[idx]; }

  private:
    SMED* m_smed;
    std::vector<T> m_items;
};

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
    // Читать из SMED значение указанного атрибута для данного Тега Сайта
    int ech_smd_p_ReadRecAttr(const char*,      // IN : site name
	               const gof_t_UniversalName,   // IN : object universal name of the record
	               const char*,                 // IN : attribut universal name
	               xdb::AttributeInfo_t* /* gof_t_StructAtt* */,            // OUT : value of the concerned attribut
	               bool&);
    // Обновить указанной Системе синтетическое состояние
    int esg_acq_dac_SmdSynthState(const char*, synthstate_t);

    // Acquisition Sites Table
    // Получить текущий список Сайтов из SMED
    SmedObjectList<AcqSiteEntry*>* SiteList() { return m_site_list; }   // Интерфейс доступа с списку Сайтов
    // Получить текущий список Запросов из SMED
    SmedObjectList<Request*>* ReqList()  { return m_request_list; }// Интерфейс доступа с списку Запросов
    SmedObjectList<Request*>* ReqDictList() { return m_request_dict_list; }// Интерфейс доступа с списку Запросов из НСИ
    // Получить текущий список Циклов из SMED
    SmedObjectList<Cycle*>* CycleList() { return m_cycle_list; }

    // Вернуть шаблон Запроса по его идентификатору
    Request* get_request_dict_by_id(ech_t_ReqId);
    // Вернуть Запрос по его идентификатору
    Request* get_request_by_id(ech_t_ReqId);

  private:
    DISALLOW_COPY_AND_ASSIGN(SMED);
    static bool m_content_initialized;
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
    // Сменить состояние всех Сайтов в "ОБРЫВ СВЯЗИ"
    int detach_sites();

    // Загрузить таблицу Сайтов из БД в память
    int store_sites();
    // Загрузить таблицу Запросов из БД в память
    int store_requests();
    int store_requests_dict();
    // Загрузить таблицу Циклов из БД в память
    int store_cycles();

    sqlite3* m_db;
    smad_connection_state_t  m_state;
    char*    m_db_err;
    EgsaConfig* m_egsa_config;  // Конфигурация нужна для восстановления структуры таблиц и загрузки начальных значений
    char*    m_snapshot_filename;

    SmedObjectList<AcqSiteEntry*> *m_site_list;    // список Сайтов в БД
    SmedObjectList<Request*>      *m_request_list; // список Запросов в БД
    SmedObjectList<Request*>      *m_request_dict_list; // НСИ список Запросов в БД
    SmedObjectList<Cycle*>        *m_cycle_list;   // список Запросов в БД

    map_id_by_name_t m_elemstruct_dict; // Связи между названием Композитной структуры и её идентификатором в БД
    map_id_by_name_t m_elemstruct_associate_dict; // Связи между ассоциацией Композитной структуры и её идентификатором в БД
    map_id_by_name_t m_sites_dict;      // Связи между названием Сайта и его идентификатором в БД

};

#endif

