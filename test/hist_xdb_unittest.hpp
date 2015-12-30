#ifndef HIST_XDB_UNITTEST_HPP
#define HIST_XDB_UNITTEST_HPP

#ifdef __cplusplus
extern "C" {
#endif

#include "mco.h"
#include "mcoxml.h"
#include "mcodbcalc.h"

#if (EXTREMEDB_VERSION <= 40)
mco_size_t file_writer(void*, const void*, mco_size_t);
#else
#include "mcouda.h"  // mco_metadict_header_t
#include "mcohv.h"   // mcohv_p
mco_size_sig_t file_writer(void*, const void*, mco_size_t);
mco_size_sig_t file_reader(void*, void*, mco_size_t);
#endif

void impl_errhandler(MCO_RET);
void extended_impl_errhandler(MCO_RET, const char*, int);

#include "xdb/impl/dat/rtap_db.h"

#ifdef __cplusplus
}
#endif

#include "xdb_common.hpp"

#define NDEV                    1
#define XDB_MAX_CONN_NUM        10
#define XDB_MAX_MEMPAGE_SIZE    256
// MSK=[Sat, 01 Jan 2000 00:00:00]
#define HIST_TEST_PERIOD_START  946674000
#define HIST_TEST_POINT_NAME    "/KD4005/GOV20/PT02"

enum ProcessingType { UNKNOWN = 0, ANALOG = 1, DISCRETE = 2, BINARY = 3 };

// ===========================================================================
static mco_calc_t      m_calc;
static mco_db_params_t m_db_params;
static mco_device_t    m_dev[NDEV];
static int             m_max_connections = XDB_MAX_CONN_NUM;
static int             m_MemoryPageSize = XDB_MAX_MEMPAGE_SIZE;
static int             m_DatabaseSize = (1024 * 1024 * 10);
//static int             m_MapAddress = 0x20000000;
static const char     *m_name = "XDB_HIST";
static int snapshot_counter = 0;
static mco_db_h        m_db;
//static mco_metadict_header_t *m_metadict;
static mco_dictionary_h       m_dict = rtap_db_get_dictionary();
//static unsigned int    m_size;
//static bool            m_metadict_initialized;

bool create_point(const char*, const objclass_t, const HistoryType);
bool create_all_hist(const char*, const HistoryType, const int);
MCO_RET create_hist_item(ProcessingType, int, HistoryType, xdb::datetime_t, autoid_t);
bool db_fill_data();
ProcessingType get_type_by_objclass(const objclass_t);
bool locate_period(const char*, time_t, const HistoryType, const int);
bool locate_samples(autoid_t, time_t, const HistoryType);

static void show_db_stat(const mco_calc_t *calc);
static const char *get_idx_type(uint4);
static void index_iterator(mco_calc_t*, mco_index_stat_t*, void*);
static void class_iterator(mco_calc_t*, mco_cc_t*, mco_cc_info_t*, void*);
#endif

