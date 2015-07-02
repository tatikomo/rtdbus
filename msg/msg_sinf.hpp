#pragma once
#ifndef MSG_SINF_HPP
#define MSG_SINF_HPP

#include <string>
#include <cstdint>

#include "config.h"
#include "msg_common.h"
#include "msg_message.hpp"

#include "xdb_rtap_const.hpp"

namespace msg {

#if 1
// Одно элементарное значение из БДРВ
typedef union
{
  int32_t     i32_value;
  int64_t     i64_value;
  float       f_value;
  double      g_value;
  std::string s_value;
} ValueItem;

// Структура-описатель элементарного тега
typedef struct
{
  std::string   tag;
  xdb::DbType_t type;
  ValueItem     value;
} UpdateItem;
#endif

typedef union {
  int32_t       val_i32;
  uint32_t      val_ui32;
  int64_t       val_i64;
  uint64_t      val_ui64;
  float         val_float;
  double        val_double;
  char*         val_char;
  std::string*  val_string;
} common_t;

////////////////////////////////////////////////////////////////////////////////////////////////////
// Хранитель значений 
////////////////////////////////////////////////////////////////////////////////////////////////////
class Value
{
  public:
    // Универсальная установка первоначальных значений
    Value(std::string&, xdb::DbType_t, void*);
    // Инициализация на основе RTDBM::ValueUpdate
    Value(const void*);
    // Установка начальных значений с неявным указанием типа
    Value(std::string&, int32_t);
    Value(std::string&, int64_t);
    Value(std::string&, float);
    Value(std::string&, double);
    Value(std::string&, std::string&);
    Value(std::string&, const char*, size_t);
    // Деструктор
   ~Value();

    // Получить название параметра
    const std::string& tag() const { return m_tag; };
    // Получить тип параметра, хранимый в БДРВ
    xdb::DbType_t type() const { return m_type; };
    const common_t& raw() const { return m_value; };
    // Инициализация на основе RTDBM::ValueUpdate
    //void CopyFrom(const void*);

  private:
    // void * RTDBM::ValueUpdate; // NB: напрямую использовать внутри вместо common_t?
    std::string     m_tag;
    xdb::DbType_t   m_type;
    common_t        m_value;
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
   ~ReadMulti();

    // Добавить в пул еще один тег
    void add(std::string&, xdb::DbType_t, void*);
//    void add(std::string&, xdb::DbType_t, std::string&);
    // Получить прямые значения параметра с заданным индексом
    bool get(std::size_t, std::string&, xdb::DbType_t&, void*);
    //const Value& operator[](std::size_t idx);
    // Получить структуру со значениями параметра с заданным индексом
    const Value& item(std::size_t idx);
    // Получить количество элементов в пуле
    int num_items();

  private:
    Value   *m_updater;
};

class WriteMulti : public Letter
{
  public:
    // Фактические значения не получены, значит нужно присвоить
    // значения по умолчанию, иначе protobuf не сериализует пустые поля
    WriteMulti();
    WriteMulti(rtdbExchangeId);
    WriteMulti(Header*, const std::string& body);
    WriteMulti(const std::string& head, const std::string& body);
   ~WriteMulti();

    // Добавить в пул еще один тег
    void add(std::string&, xdb::DbType_t, void*);
//    void add(std::string&, xdb::DbType_t, std::string&);
    // Получить значения параметра по заданному индексу
    bool get(std::size_t, std::string&, xdb::DbType_t&, void*);
    // Получить структуру со значениями параметра с заданным индексом
    const Value& item(std::size_t idx);

    // Получить количество элементов в пуле
    int num_items();

  private:
    Value   *m_updater;
};
 
} // namespace msg

#endif

