#ifndef EXCHANGE_SMED_HXX
#define EXCHANGE_SMED_HXX

#include <string>
#include <memory>

#include "odb/core.hxx"

// NB: Структура таблиц содержится в файле exchange_smed.hpp

class REQUESTS;
class SITES;
class CYCLES;
class SITE_CYCLE_LINK;
class DATA;
class FIELDS_LINK;
class FIELDS;
class ELEMTYPE;
class ASSOCIATES_LINK;
class ELEMSTRUCT;

// ==========================================================================================================
#pragma db object
class REQUESTS
{
  public:
    REQUESTS(int _state, int _start, int _finish, int _type, int _uid, int _nature, const std::string& _data)
    : m_state(_state),
      m_date_start(_start),
      m_date_finish(_finish),
      m_type(_type),
      m_uid(_uid),
      m_data_name(_data)
    {}
  private:
    REQUESTS() {}
    friend class odb::access;
    #pragma db id auto
    #pragma db column("REQ_ID")
    unsigned long m_req_id;
    #pragma db column("STATE")
    int m_state;
    #pragma db column("DATE_START")
    int m_date_start;
    #pragma db column("DATE_FINISH")
    int m_date_finish;
    #pragma db column("TYPE")
    int m_type;
    #pragma db column("UID")
    int m_uid;
    #pragma db column("DATA_NAME")
    std::string m_data_name;
    #pragma db not_null
    #pragma db column("SITE_REF")
    std::shared_ptr<SITES> m_site_ref;
    #pragma db not_null
    #pragma db column("CYCLE_REF")
    std::shared_ptr<CYCLES> m_cycle_ref;
};

// ==========================================================================================================
#pragma db object
class SITES
{
  public:
    SITES(const std::string& _name, int _synthstate, int _expmode, int _inhibition, int _nature, int _last_update)
      : m_name(_name),
        m_synthstate(_synthstate),
        m_expmode(_expmode),
        m_inhibition(_inhibition),
        m_nature(_nature),
        m_last_update(_last_update)
    {}

    const std::string& name () const
    {
      return m_name;
    }

  private:
    SITES()
      : m_name(),
        m_synthstate(0),
        m_expmode(0),
        m_inhibition(0),
        m_nature(0),
        m_last_update(0)
    {}
    friend class odb::access;
    #pragma db id auto
    #pragma db column("SITE_ID")
    unsigned long m_site_id;
    #pragma db column("NAME")
    std::string m_name;
    #pragma db column("SYNTHSTATE")
    int m_synthstate;
    #pragma db column("EXPMODE")
    int m_expmode;
    #pragma db column("INHIBITION")
    int m_inhibition;
    #pragma db column("NATURE")
    int m_nature;
    #pragma db column("LAST_UPDATE")
    int m_last_update;
};

// ==========================================================================================================
#pragma db object
class CYCLES
{
  public:
    CYCLES(const std::string& _name, int _family, int _period, int _type)
      : m_name(_name),
        m_family(_family),
        m_period(_period),
        m_type(_type)
    {}
  private:
    CYCLES() {}
    friend class odb::access;
    #pragma db id auto
    #pragma db column("CYCLE_ID")
    unsigned long m_cycle_id;
    #pragma db column("NAME")
    std::string m_name;
    #pragma db column("FAMILY")
    int m_family;
    #pragma db column("PERIOD")
    int m_period;
    #pragma db column("TYPE")
    int m_type;
};

#if 1
// ==========================================================================================================
#pragma db object
class SITE_CYCLE_LINK
{
  public:
    SITE_CYCLE_LINK(std::shared_ptr<SITES> _site, std::shared_ptr<CYCLES> _cycle)
      : m_site(_site),
        m_cycle(_cycle)
    {}
    unsigned long id() { return m_sa_cyc_id; }
    std::shared_ptr<SITES> site() { return m_site; }
    std::shared_ptr<CYCLES> cycle() { return m_cycle; }

  private:
    SITE_CYCLE_LINK() {}
    friend class odb::access;
    #pragma db id auto column("SA_CYC_ID")
    unsigned long m_sa_cyc_id;
    #pragma db column("SITE_REF") not_null
    std::shared_ptr<SITES> m_site;
    #pragma db column("CYCLE_REF") not_null
    std::shared_ptr<CYCLES> m_cycle;
};
#endif

// ==========================================================================================================
#pragma db object
class DATA
{
  public:
    DATA(const std::string& _tag, int _led, int _objclass, int _category, int _alarm_indic = 0, int _histo_indic = 0)
      : m_tag(_tag),
        m_led(_led),
        m_objclass(_objclass),
        m_category(_category),
        m_alarm_indic(_alarm_indic),
        m_histo_indic(_histo_indic)
    {}
    const std::string& tag() const { return m_tag; }
    int id() const  { return m_data_id; }
    int led() const { return m_led; }
    int objclass() const { return m_objclass; }
    int category() const { return m_category; }
    int alarm_indic() const { return m_alarm_indic; }
    int histo_indic() const { return m_histo_indic; }
  private:
    DATA() {}
    friend class odb::access;
    #pragma db id auto column("DATA_ID")
    unsigned long m_data_id;
    #pragma db column("TAG") not_null
    std::string m_tag;
    #pragma db column("LED")
    int m_led;
    #pragma db column("OBJCLASS")
    int m_objclass;
    #pragma db column("CATEGORY")
    int m_category;
    #pragma db column("ALARM_INDIC")
    int m_alarm_indic;
    #pragma db column("HISTO_INDIC")
    int m_histo_indic;
};

// ==========================================================================================================
#pragma db object
class FIELDS_LINK
{
  public:
    FIELDS_LINK(std::shared_ptr<DATA> _data,
                std::shared_ptr<ELEMSTRUCT> _elemstruct,
                std::shared_ptr<FIELDS> _field,
                int _last_update,
                int _need_sync)
      : m_elemstruct(_elemstruct),
        m_field(_field),
        m_data(_data),
        m_last_update(_last_update),
        m_need_sync(_need_sync)
    {}
    int get_integer() const { return m_val_int; }
    double get_double() const { return m_val_double; }
    const std::string& get_string() const { return m_val_str; }
    unsigned long get_link_id() const { return m_link_id; }
    std::shared_ptr<ELEMSTRUCT> elemstruct() { return m_elemstruct; }
    std::shared_ptr<FIELDS> field() { return m_field; }
    std::shared_ptr<DATA> data() { return m_data; }
    int need_sync() const { return m_need_sync; }
    int last_update() const { return m_last_update; }

  private:
    FIELDS_LINK() {}
    friend class odb::access;
    #pragma db id auto
    #pragma db column("LINK_ID")
    unsigned long m_link_id;

    #pragma db column("ELEMSTRUCT_REF") not_null
    std::shared_ptr<ELEMSTRUCT> m_elemstruct;
    #pragma db column("FIELD_REF") not_null
    std::shared_ptr<FIELDS> m_field;
    #pragma db column("DATA_REF") not_null
    std::shared_ptr<DATA> m_data;
    #pragma db column("LAST_UPDATE")
    int m_last_update;
    #pragma db column("NEED_SYNC")
    int m_need_sync;
    #pragma db column("VAL_INT")
    int m_val_int;
    #pragma db column("VAL_DOUBLE")
    double m_val_double;
    #pragma db column("VAL_STR")
    std::string m_val_str;
};

// ==========================================================================================================
#pragma db object no_id
class ASSOCIATES_LINK
{
  public:
    ASSOCIATES_LINK(std::shared_ptr<DATA> _data, std::shared_ptr<ELEMSTRUCT> _elemstruct)
      : m_data(_data),
        m_elemstruct(_elemstruct)
    {}
    std::shared_ptr<DATA> data() { return m_data; }
    std::shared_ptr<ELEMSTRUCT> elemstruct() { return m_elemstruct; }
  private:
    ASSOCIATES_LINK() {}
    friend class odb::access;
    #pragma db column("DATA_REF")
    std::shared_ptr<DATA> m_data;
    #pragma db column("ELEMSTR_REF")
    std::shared_ptr<ELEMSTRUCT> m_elemstruct;
};

// ==========================================================================================================
#pragma db object
class ELEMSTRUCT
{
  public:
  private:
    ELEMSTRUCT() {}
    friend class odb::access;
    #pragma db id auto
    #pragma db column("STRUCT_ID")
    unsigned long m_struct_id;
    #pragma db column("NAME") not_null
    std::string m_name;       // Название Композитной структуры
    #pragma db column("ASSOCIATE") not_null
    std::string m_associate;  // Название Ассоциации
    #pragma db column("CLASS")
    int m_class;
};

// ==========================================================================================================
// Таблица FIELDS набора словарных полей
#pragma db object
class FIELDS
{
  public:
    FIELDS(std::shared_ptr<ELEMSTRUCT> _elemstruct,
           std::shared_ptr<ELEMTYPE> _elemtype,
           const std::string& _attr,
           int _type,
           int _length)
      : m_elemstruct(_elemstruct),
        m_elemtype(_elemtype),
        m_attr(_attr),
        m_type(_type),
        m_length(_length)
    {}
  private:
    FIELDS() {}
    friend class odb::access;
    #pragma db id auto
    #pragma db column("FIELD_ID")
    unsigned long m_field_id;
    #pragma db column("ELEMSTRUCT_REF") not_null
    std::shared_ptr<ELEMSTRUCT> m_elemstruct;   // Ссылка на родительский элемент Композитной структуры
    #pragma db column("ELEMTYPE_REF") not_null
    std::shared_ptr<ELEMTYPE> m_elemtype;       // Ссылка на используемый Базовый элемент Композитной структуры
    #pragma db column("ATTR")
    std::string m_attr;               // Название в БДРВ атрибута параметра
    #pragma db column("TYPE")
    int m_type;                       // Тип в БДРВ атрибута параметра
    #pragma db column("LENGTH")
    int m_length;                     // Длина (имеет смысл только для строкового типа)
};

// ==========================================================================================================
// Таблица Базовых элементов ELEMTYPE
#pragma db object
class ELEMTYPE
{
  public:
  private:
    ELEMTYPE() {}
    friend class odb::access;
    #pragma db id auto
    #pragma db column("TYPE_ID")
    unsigned long m_type_id;
    #pragma db column("NAME") not_null
    std::string m_name;       // Название элемента (пример: "L1800")
    #pragma db column("TYPE")
    int m_type;               // Тип данных элемента (числовой код типов Real, Int, String, Time, Logic)
    #pragma db column("SIZE")
    std::string m_size;       // Формат размерности элемента (к примеру "11e4", "2.2", "U20", "8", ...)
};

// ==========================================================================================================

#endif  // EXCHANGE_SMED_HXX

