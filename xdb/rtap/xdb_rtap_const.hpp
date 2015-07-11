#pragma once
#ifndef XDB_RTAP_CONST_HPP
#define XDB_RTAP_CONST_HPP

#if defined HAVE_CONFIG_H
#include "config.h"
#endif

#include "xdb_common.hpp"

namespace xdb {

#define D_MISSING_OBJCODE   "MISSING"

// ------------------------------------------------------------

#if 0
extern xdb::ClassDescription_t ClassDescriptionTable[];
extern const xdb::DeTypeDescription_t rtDataElem[];
// Получить универсальное имя на основе его алиаса
extern int GetPointNameByAlias(univname_t&, univname_t&);
// Хранилище описаний типов данных БДРВ
extern const xdb::DbTypeDescription_t DbTypeDescription[];
extern const xdb::DeTypeToDbTypeLink  DeTypeToDbType[];
#endif
// ------------------------------------------------------------

}; // namespace xdb

#endif

