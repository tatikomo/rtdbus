#pragma once
#ifndef XDB_DATABASE_RTAP_SBS_HPP
#define XDB_DATABASE_RTAP_SBS_HPP

#include "xdb_common.hpp"

namespace xdb {

#define IDX_SBS_REAL_ATTRIB_MAX 10
// Соответствие между количеством атрибутов и набором атрибутов
// Используется для быстрого определения значимых атрибутов при
// чтении списка атрибутов для передачи подписчику группы
typedef struct {
 int objclass;
 int quantity; // размер списка (<IDX_SBS_REAL_ATTRIB_MAX)
 int list[IDX_SBS_REAL_ATTRIB_MAX];   // список индексов атрибутов
} read_attribs_on_sbs_event_t;


// Получение информации по атрибутам, которые необходимо включить в
// рассылку группы подписки.
// Для аналоговых и дискретных Точек используется разный репертуар.
// Получив на входе OBJCLASS Точки, вернуть для нее список
// атрибутов с количеством.
class AttributesHolder
{
  public:
    AttributesHolder();
   ~AttributesHolder();

    const read_attribs_on_sbs_event_t& info(int /* objclass */);
//            RTDB_ATT_IDX_TAG,
//            RTDB_ATT_IDX_OBJCLASS,
  private:
};

}; // end namespace xdb

#endif

