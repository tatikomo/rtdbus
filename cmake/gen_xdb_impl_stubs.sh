#!/bin/bash
# Скрипт генерации определений и пустых реализаций функций для
# создания/записи/чтения атрибутов.
#
# Вызывается из процедуры генерации проекта. Зависит от набора атрибутов,
# находящихся в файле xdb/impl/dat/rtap_db.xsd
# Если для какого-либо атрибута в файле xdb/impl/xdb_impl_db_rtap.cpp уже
# имеется функция прототипа, этот атрибут пропускается.
#
# Рабочий каталог
# ${XDB_IMPL_DIR}/dat
#
# Выходные файлы
#   proto_attr_reading.gen      : прототипы функций чтения атрибутов
#   proto_attr_writing.gen      : прототипы функций записи атрибутов
#   proto_attr_creating.gen     : прототипы функций создания атрибутов
#   impl_attr_creating_map.gen  : реализация заполения хеша функциями создания атрибутов
#   impl_attr_reading_map.gen   : реализация заполения хеша функциями чтения атрибутов
#   impl_attr_writing_map.gen   : реализация заполения хеша функциями записи атрибутов
#   impl_func_read.gen          : реализация функций чтения атрибутов
#   impl_func_write.gen         : реализация функций записи атрибутов
#
# Дата создания: 2015/07/09
#

cat attr_list.txt | grep -v ^# | \
while read a; do \
  echo "MCO_RET create$a (PointInDatabase*, rtap_db::Attrib&);"; \
done > proto_attr_creating.gen

cat attr_list.txt | grep -v ^# | \
while read a; do \
  echo "MCO_RET read$a (mco_trans_h&, rtap_db::XDBPoint&, AttributeInfo_t*);"; \
done > proto_attr_reading.gen

cat attr_list.txt | grep -v ^# | \
while read a; do \
  echo "MCO_RET write$a (mco_trans_h&, rtap_db::XDBPoint&, AttributeInfo_t*);"; \
done > proto_attr_writing.gen

cat attr_list.txt | grep -v ^# | \
while read a; do \
  echo "  m_attr_creating_func_map.insert(AttrCreatingFuncPair_t(\"$a\", &xdb::DatabaseRtapImpl::create$a));"; \
done > impl_attr_creating_map.gen

cat attr_list.txt | grep -v ^# | \
while read a; do \
  echo "  m_attr_reading_func_map.insert(AttrProcessingFuncPair_t(\"$a\", &xdb::DatabaseRtapImpl::read$a));"; \
done > impl_attr_reading_map.gen

cat attr_list.txt | grep -v ^# | \
while read a; do \
  echo "  m_attr_writing_func_map.insert(AttrProcessingFuncPair_t(\"$a\", &xdb::DatabaseRtapImpl::write$a));"; \
done > impl_attr_writing_map.gen

cat attr_list.txt | grep -v ^# | \
while read a; do \
  exist=$(grep "::read$a(" ../xdb_impl_db_rtap.cpp); \
  [ "x$exist" == "x" ] && echo -e "MCO_RET DatabaseRtapImpl::read$a(mco_trans_h&, rtap_db::XDBPoint&, AttributeInfo_t*)\n{\n  MCO_RET rc = MCO_S_OK;\n  return rc;\n}\n"; \
done > impl_func_read.gen

cat attr_list.txt | grep -v ^# | \
while read a; do \
  exist=$(grep "::write$a(" ../xdb_impl_db_rtap.cpp); \
  [ "x$exist" == "x" ] && echo -e "MCO_RET DatabaseRtapImpl::write$a(mco_trans_h&, rtap_db::XDBPoint&, AttributeInfo_t*)\n{\n  MCO_RET rc = MCO_S_OK;\n  return rc;\n}\n"; \
done > impl_func_write.gen

exit 0
