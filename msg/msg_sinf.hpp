#pragma once
#ifndef MSG_SINF_HPP
#define MSG_SINF_HPP

#if defined HAVE_CONFIG_H
#include "config.h"
#endif

#include <string>
#include <cstdint>

#include "config.h"
#include "msg_common.h"
#include "msg_message.hpp"

#include "xdb_common.hpp"

namespace msg {

// Элементарная запись единичного семпла
typedef struct {
  time_t datehourm;
  double val;
  char valid;
} hist_attr_t;

////////////////////////////////////////////////////////////////////////////////////////////////////
// Хранитель значений 
////////////////////////////////////////////////////////////////////////////////////////////////////
class Value
{
  public:
    // Универсальная установка первоначальных значений
    Value(std::string&, xdb::DbType_t, void*);
    // Инициализация на основе RTDBM::ValueUpdate
    Value(void*);
    // Установка начальных значений с неявным указанием типа
    Value(std::string&, bool);
    Value(std::string&, int8_t);
    Value(std::string&, uint8_t);
    Value(std::string&, int16_t);
    Value(std::string&, uint16_t);
    Value(std::string&, int32_t);
    Value(std::string&, uint32_t);
    Value(std::string&, int64_t);
    Value(std::string&, uint64_t);
    Value(std::string&, float);
    Value(std::string&, double);
    Value(std::string&, std::string&);
    Value(std::string&, const char*, size_t);
    // Деструктор
   ~Value();

    // Получить название параметра
    const std::string& tag() const { return m_instance.name; };
    // Получить тип параметра, хранимый в БДРВ
    xdb::DbType_t type() const { return m_instance.type; };
    const xdb::AttrVal_t& raw() const { return m_instance.value; };
    xdb::AttributeInfo_t& instance() { return m_instance; };
    // вернуть значение в строковом виде
    const std::string as_string() const;
    // Сбросить значения из БДРВ в RTDBM::ValueUpdate
    void flush();
    // Инициализация на основе RTDBM::ValueUpdate
    //void CopyFrom(const void*);

  private:
    DISALLOW_COPY_AND_ASSIGN(Value);
    // void * RTDBM::ValueUpdate; // NB: напрямую использовать внутри вместо AttrVal_t?
    xdb::AttributeInfo_t m_instance;
    // Ссылка на RTDBM::ValueUpdate для синхронизации изменений данных
    void* m_rtdbm_valueupdate_instance;
};

////////////////////////////////////////////////////////////////////////////////////////////////////
// Сообщения, участвующие в общем взаимодействии процессов [ADG_D_MSG_*]
// TODO: внедрить автогенерацию файлов по описателям в msg/proto/common.proto
// Список:
//
////////////////////////////////////////////////////////////////////////////////////////////////////
class ReadMulti : public Letter
{
  public:
    ReadMulti();
    ReadMulti(rtdbExchangeId);
    ReadMulti(Header*, const std::string& body);
    ReadMulti(const std::string& head, const std::string& body);
    virtual ~ReadMulti();

    // Добавить в пул еще один тег
    void add(std::string&, xdb::DbType_t, void*);
    //void add(std::string&, xdb::DbType_t, std::string&);

    // Получить прямые значения параметра с заданным индексом
    bool get(std::size_t, std::string&, xdb::DbType_t&, xdb::Quality_t&, void*);
    bool get(std::size_t, std::string&, xdb::DbType_t&, xdb::Quality_t&, xdb::AttrVal_t&);

    //const Value& operator[](std::size_t idx);
    // Получить структуру со значениями параметра с заданным индексом
    Value& item(std::size_t idx);

    // Явно установить заданный тип атрибута
    void set_type(std::size_t idx, xdb::DbType_t);

    // Получить количество элементов в пуле
    std::size_t num_items();

  private:
    DISALLOW_COPY_AND_ASSIGN(ReadMulti);
    Value   *m_updater;
};

////////////////////////////////////////////////////////////////////////////////////////////////////
class WriteMulti : public Letter
{
  public:
    // Фактические значения не получены, значит нужно присвоить
    // значения по умолчанию, иначе protobuf не сериализует пустые поля
    WriteMulti();
    WriteMulti(rtdbExchangeId);
    WriteMulti(Header*, const std::string&);
    WriteMulti(const std::string&, const std::string&);
    virtual ~WriteMulti();

    // Добавить в пул еще один тег
    void add(std::string&, xdb::DbType_t, void*);
//    void add(std::string&, xdb::DbType_t, std::string&);
    // Получить значения параметра по заданному индексу
    bool get(std::size_t, std::string&, xdb::DbType_t&, void*);
    // Получить структуру со значениями параметра с заданным индексом
    Value& item(std::size_t idx);

    // Получить количество элементов в пуле
    std::size_t num_items();

  private:
    DISALLOW_COPY_AND_ASSIGN(WriteMulti);
    Value   *m_updater;
};

////////////////////////////////////////////////////////////////////////////////////////////////////
class SubscriptionControl : public Letter
{
  public:
    SubscriptionControl();
    SubscriptionControl(const std::string&, const std::string&);
    SubscriptionControl(Header*, const std::string&);
    virtual ~SubscriptionControl();
    // Установить состояние Группе
    void set_ctrl(int);
    // Вернуть код текущего состояния Группы
    int ctrl();
    // Установить имя Группы
    void set_name(std::string&);
    const std::string& name();

  private:
    DISALLOW_COPY_AND_ASSIGN(SubscriptionControl);
    Value   *m_updater;
};

////////////////////////////////////////////////////////////////////////////////////////////////////
class SubscriptionEvent : public Letter
{
  public:
    // Фактические значения не получены, значит нужно присвоить
    // значения по умолчанию, иначе protobuf не сериализует пустые поля
    SubscriptionEvent();
    SubscriptionEvent(const std::string&, const std::string&);
    SubscriptionEvent(Header*, const std::string&);
    virtual ~SubscriptionEvent();

    // Добавить в пул еще один тег
    void add(std::string&, xdb::DbType_t, void*);
    // Получить значения параметра по заданному индексу
    bool get(std::size_t, std::string&, xdb::DbType_t&, xdb::Quality_t&, xdb::AttrVal_t&);
    // Получить структуру со значениями параметра с заданным индексом
    Value& item(std::size_t idx);

    // Получить количество элементов в пуле
    std::size_t num_items();

  private:
    DISALLOW_COPY_AND_ASSIGN(SubscriptionEvent);
    Value   *m_updater;
};
 
////////////////////////////////////////////////////////////////////////////////////////////////////
class HistoryRequest : public Letter
{
  public:
    // Фактические значения не получены, значит нужно присвоить
    // значения по умолчанию, иначе protobuf не сериализует пустые поля
    HistoryRequest();
    HistoryRequest(const std::string&, const std::string&);
    HistoryRequest(Header*, const std::string&);
    virtual ~HistoryRequest();

    // Указать тег для получения истории
    void set(std::string&, time_t /* start */, int /* num_samples */, int /* history_type */);
    // Добавить в пул еще одну запись
    void add(time_t, double, int);
    // Получить значения записи с заданным индексом
    bool get(int, hist_attr_t&);

    // методы доступа
    const std::string& tag();
    time_t start_time();
    // Получить количество запрошенных элементов
    int num_required_samples();
    // Получить количество действительных элементов в пуле
    int num_read_samples();
    xdb::sampler_type_t history_type();

  private:
    DISALLOW_COPY_AND_ASSIGN(HistoryRequest);
};
 

} // namespace msg


#endif

