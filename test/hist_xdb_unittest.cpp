/*
g++ -g xdb-hist-test.cpp -DEXTREMEDB_VERSION=51 -o xdb-hist-test xdb/impl/dat/rtap_db.h xdb/impl/dat/rtap_db.c -I../eXtremeDB_5_fusion_eval/include -L../eXtremeDB_5_fusion_eval/target/bin.so -lmcolib_debug -lmcovtmem_debug -lmcofu98_debug -lmcoslnx_debug -lmcomipc_debug -lmcotmvcc_debug -lmcolib_debug -lmcouwrt_debug -lmcohv_debug -lmcouda_debug -lmcoews_cgi_debug -pthread

LD_LIBRARY_PATH=$LD_LIBRARY_PATH:/home/gev/ITG/sandbox/eXtremeDB_5_fusion_eval/target/bin.so ./xdb-hist-test
 */
#include <iostream>

#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#ifdef __cplusplus
extern "C" {
#endif

#include "mco.h"
#include "mcoxml.h"
#include "mcodbcalc.h"

#ifdef __cplusplus
}
#endif

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "glog/logging.h"
#include "gtest/gtest.h"

#include "hist_xdb_unittest.hpp"
#include "xdb_common.hpp"

// ===========================================================================
#if (EXTREMEDB_VERSION <= 40)
mco_size_t
#else
mco_size_sig_t
#endif
file_writer( void *stream_handle /* FILE *  */, const void *from, mco_size_t nbytes )
{
  FILE *f = (FILE *)stream_handle;
  mco_size_sig_t nbs;

  nbs = fwrite( from, 1, nbytes, f );
  return nbs;
}

/* Stream reader with prototype mco_stream_read */
#if (EXTREMEDB_VERSION <= 40)
mco_size_t
#else
mco_size_sig_t
#endif
file_reader( void *stream_handle /* FILE *  */,  /* OUT */void *to, mco_size_t max_nbytes )
{
  FILE *f = (FILE *)stream_handle;
  mco_size_sig_t nbs;

  nbs = fread( to, 1, max_nbytes, f );
  return nbs;
}

void impl_errhandler(MCO_RET n)
{
    fprintf(stdout, "\neXtremeDB runtime fatal error: %d\n", n);
    exit(-1);
}

/* implement error handler */
void extended_impl_errhandler(MCO_RET errcode, const char* file, int line)
{
  fprintf(stdout, "\neXtremeDB runtime fatal error: %d on line %d of file %s",
          errcode, line, file);
  exit(-1);
}

// Установка общих значимых полей для эксорта/импорта данных в XML
void setupPolicy(mco_xml_policy_t& policy)
{
  policy.blob_coding = MCO_TEXT_BASE64; //MCO_TEXT_BINHEX
  // encode line feeds
  policy.encode_lf   = MCO_YES;
  // encode national chars, MCO_YES ПРИВЕДЕТ К ЧИСЛОВОЙ КОДИРОВКЕ РУССКИХ БУКВ
  policy.encode_nat  = MCO_NO;
  policy.encode_spec = MCO_YES;
  policy.float_format= MCO_FLOAT_FIXED;
  policy.ignore_field= MCO_YES;
  policy.indent      = MCO_YES;
  policy.int_base    = MCO_NUM_DEC;
  policy.quad_base   = MCO_NUM_HEX; /* other are invalid */
  policy.text_coding = MCO_TEXT_ASCII;
  policy.truncate_sp = MCO_YES;
  policy.use_xml_attrs  = MCO_NO; //YES;
  policy.ignore_autoid  = MCO_NO;
  policy.ignore_autooid = MCO_NO;
}

static void show_db_stat(const mco_calc_t *calc)
{
  printf("Database memory statistics:\n");
  printf("Page size: %d\n", calc->pg_size);
  printf("Free pages: %d\n", calc->free_pgs);
  printf("Total pages: %d\n", calc->total_pgs);
  printf("Bytes used: %d\n", (calc->total_pgs - calc->free_pgs) * calc->pg_size);
  printf("Registered classes: %d\n", calc->ncls);
}

static void class_iterator(mco_calc_t *calc, mco_cc_t *cls, mco_cc_info_t *info, void *priv_data)
{
  printf("%s statistics:\n", cls->cc_name);
  printf(" - objects: %d\n", info->nobjs);
  printf(" - bytes min: %lld\n", info->bytes_min);
  printf(" - bytes cur: %lld\n", info->bytes_cur);
  printf(" - bytes max: %lld\n", info->bytes_max);
  printf(" - pages min: %d\n", info->pages_min);
  printf(" - pages cur: %d\n", info->pages_cur);
  printf(" - pages max: %d\n", info->pages_max);
  printf("%s has %d strings, %d vectors and %d blobs\n",
         cls->cc_name, info->nstrs, info->nvecs, info->nblobs);
  printf("Filling time: %ld\n", cls->cc_time);
}

static const char *get_idx_type(uint4 type)
{
  switch( type & MCO_IDXST_FUNCTION_MASK ) {
    case MCO_IDXST_FUNCTION_REGULAR:
      return "Regular";
    case MCO_IDXST_FUNCTION_OID:
      return "OID";
    case MCO_IDXST_FUNCTION_AUTOOID:
      return "AUTOOID";
    case MCO_IDXST_FUNCTION_AUTOID:
      return "AutoID";
    case MCO_IDXST_FUNCTION_LIST:
      return "List";
    default:
      return "Unknown";
  }
}

/* iterator which will be used with mco_calc_iinfo_brouse */
static void index_iterator(mco_calc_t *calc, mco_index_stat_t *istat, void *priv_data)
{
  int tmp;
  char *str = NULL, c;

  if ((istat->type & MCO_IDXST_TYPE_MASK) == MCO_IDXST_TYPE_MEM)
    str = (const char*) "Inmem";
  else if ((istat->type & MCO_IDXST_TYPE_MASK) == MCO_IDXST_TYPE_DISK)
    str = (const char*) "Disk";

  printf(" -> %s [%s ", istat->plabel, str);

  if (istat->type & MCO_IDXST_FEATURE_UNIQUE)
    printf("unique ");

  if (istat->type & MCO_IDXST_FEATURE_UDF)
    printf("userdef ");

  tmp = 0;

  switch (istat->type & MCO_IDXST_NATURE_MASK) {
    case MCO_IDXST_NATURE_BTREE:
      tmp = istat->spec.btree.levels_num;
      printf("btree");
      break;

    case MCO_IDXST_NATURE_PTREE:
      tmp = istat->spec.ptree.levels_num;
      printf("ptree");
      break;

    case MCO_IDXST_NATURE_RTREE:
      tmp = istat->spec.rtree.levels_num;
      printf("rtree");
      break;

    case MCO_IDXST_NATURE_HASH:
      printf("hash");
      break;

    case MCO_IDXST_NATURE_META:
      printf("meta");
      break;

    default:
      printf("unknown");
  }

  printf("]\n");
  printf(" * %-20s %s\n", "Type:", get_idx_type(istat->type));

  switch (istat->type & MCO_IDXST_NATURE_MASK) {
    case MCO_IDXST_NATURE_BTREE:
    case MCO_IDXST_NATURE_PTREE:
    case MCO_IDXST_NATURE_RTREE:
    {
      printf(" * %-20s %8d\n", "Levels:", tmp);
      break;
    }

    case MCO_IDXST_NATURE_HASH:
      printf(" * %-20s %8ld\n", "Avg chain length:", (long)istat->spec.hash.avg_chain_length);
      printf(" * %-20s %8ld\n", "Max chain length:", (long)istat->spec.hash.max_chain_length);
      break;

    case MCO_IDXST_NATURE_META:
      printf(" * %-20s %8ld\n", "Disk pages:", (long)istat->spec.meta.disk_pages_num);
      break;
  }

  printf(" * %-20s %8ld\n", "Keys:", (long)istat->keys_num);
  printf(" * %-20s %8ld\n", "Pages:", (long)istat->pages_num);
  tmp = istat->pages_num * calc->pg_size;

  if (tmp > 0x400) {
    tmp >>= 10;
    c = 'K';
  }
  else c = 'B';

  printf(" * %-20s %8d%c\n", "Space:", tmp, c);
}

/*
 * Сохранение содержимого БД в виде файла XML формата McObject.
 *
 * NB: Внутри функции может упасть xdb runtime в том случае, если 
 * сохраняемая БД имеет хотя бы один безиндексный класс.
 */
bool SaveAsXML()
{
  bool status = false;
  MCO_RET rc = MCO_S_OK;
  mco_xml_policy_t op, np;
  mco_trans_h t;
  static char calc_file_name[150];
  char *fname = &calc_file_name[0];
  FILE* f;

  snprintf(calc_file_name, sizeof(calc_file_name)-1,
          "%09d.%s.snap.xml",
          snapshot_counter++,
          m_name);
    
  /* setup policy */
  setupPolicy(np);

  f = fopen(fname, "wb");
  if (!f)
  {
    std::cout << "E Unable to open XML-snapshot file " << fname << std::endl;
    return false;
  }
  /* export content of the database to a file */
  rc = mco_trans_start(m_db, MCO_READ_WRITE, MCO_TRANS_HIGH, &t);

  if (rc == MCO_S_OK)
  {
       /* setup XML subsystem*/
      mco_xml_get_policy(t, &op);
      rc = mco_xml_set_policy(t, &np);
      if (MCO_S_OK != rc)
      {
        std::cout<< "E Unable to set xml policy, rc=" << rc << std::endl;
      }
      rc = mco_db_xml_export(t, f, file_writer);

      /* revert xml policy */
      mco_xml_set_policy(t, &op);

      if (rc != MCO_S_OK)
      {
         std::cout << "E Exporting, rc=" << rc << std::endl;
         mco_trans_rollback(t);
      }
      else
      {
         rc = mco_trans_commit(t);
         if (rc) {
           std::cout << "E Commitment transaction, rc=" << rc << std::endl;
         }
         else {
           status = true;
         }
      }
  }
  else
  {
    std::cout << "E Opening transaction, rc=" << rc << std::endl;
  }

  fclose(f);
  return status;
}
// ===========================================================================


// ===========================================================================

bool db_connect()
{
  static const char* fname = "db_connect";
  bool result = true;
  MCO_RET rc = MCO_S_OK;
  mco_runtime_info_t info;

  rc = mco_runtime_start();

  if (rc) {
    std::cout << "F " << fname << ": mco_runtime_start, rc=" << rc << std::endl;
    return false;
  }

  mco_get_runtime_info(&info);
  if (!info.mco_shm_supported) {
    std::cout << "F " << fname << ": shared memory db is not supported" << std::endl;
    return false;
  }

  memset(static_cast<void*>(&m_dev), '\0', sizeof(mco_device_t) * NDEV);
  /* setup memory device as a shared named memory region */
  m_dev[0].assignment = MCO_MEMORY_ASSIGN_DATABASE;
  m_dev[0].size       = m_DatabaseSize;
  m_dev[0].type       = MCO_MEMORY_NAMED; /* DB in shared memory */
  sprintf(m_dev[0].dev.named.name, "%s-db", m_name);
  m_dev[0].dev.named.flags = 0;
  m_dev[0].dev.named.hint  = 0;

  mco_db_params_init (&m_db_params);
  m_db_params.db_max_connections       = m_max_connections;
  m_db_params.connection_context_size  = sizeof(pid_t);
  /* set page size for in memory part */
  m_db_params.mem_page_size      = m_MemoryPageSize;

  rc = mco_db_kill(m_name);
  if (rc && (MCO_E_NOINSTANCE != rc)) {
    std::cout << "W " << fname << ": mco_db_kill '" << m_name << "', rc=" << rc << std::endl;
  }

  mco_error_set_handler_ex(&extended_impl_errhandler);

  mco_calc_init(&m_calc, m_dict);

#if 1
  rc = mco_db_open_dev(m_name,
                       m_dict,
                       m_dev,
                       NDEV,
                       &m_db_params);
  if (rc) {
    std::cout << "F " << fname << ": mco_db_open_dev '" << m_name << "', rc=" << rc << std::endl;
    return false;
  }

  rc = mco_db_connect(m_name, &m_db);
  if (rc) {
    std::cout << "F " << fname << ": mco_db_connect '" << m_name << "', rc=" << rc << std::endl;
    return false;
  }

  rc = mco_calc_reg_schema_classes(&m_calc, rtap_db_get_calculator());
  if (rc) {
    std::cout << "W " << fname << ": mco_calc_reg_schema_classes '" << m_name << "', rc=" << rc << std::endl;
  }
  else {
    m_calc.db = m_db;

    m_calc.dsl.v_lmin = 10;
    m_calc.dsl.v_lmax = 40;
    m_calc.dsl.s_lmin = 10;
    m_calc.dsl.s_lmax = 100;

    rc = mco_calc_fill_db(&m_calc);

#if 0
    if (rc) {
      std::cout << "W " << fname << ": mco_calc_fill_db '" << m_name << "', rc=" << rc << std::endl;
    }
    else {
      /*
       * Note, mco_stat_collect must be called before
       * mco_calc_iinfo_brouse or mco_calc_cinfo_brouse
       */
      rc = mco_calc_stat_collect(&m_calc);

      if (rc == MCO_S_OK) {

        show_db_stat(&m_calc);

        rc = mco_calc_cinfo_browse(&m_calc, class_iterator, NULL);
        if (rc) {
          std::cout << "W " << fname << ": mco_calc_cinfo_brouse, rc=" << rc << std::endl;
        }
        rc = mco_calc_iinfo_browse(&m_calc, index_iterator, NULL);
        if (rc) {
          std::cout << "W " << fname << ": mco_calc_iinfo_brouse, rc=" << rc << std::endl;
        }
      }
    }
#endif
  }
#else

  /* initialize MCOHV */
  m_hv = 0;
  int ret = mcohv_initialize();
  std::cout << "I mcohv_initialize '" << m_name << "', rc=" << ret << std::endl;

  mco_metadict_size(1, &m_size);
  m_metadict = (mco_metadict_header_t *) malloc(m_size);
  rc = mco_metadict_init (m_metadict, m_size, 0);
  std::cout << "I mco_metadict_init '" << m_name << ", rc=" << rc << std::endl;
  if (rc)
  {
    result = false;
    std::cout << "E Unable to initialize UDA metadictionary, rc=" << rc << std::endl;
    free(m_metadict);
    m_metadict_initialized = false;
  }
  else
  {
    result = true;
    m_metadict_initialized = true;
    rc = mco_metadict_register(m_metadict, m_name, m_dict, NULL);
    std::cout << "I mco_metadict_register '" << m_name << ", rc=" << rc << std::endl;
    if (rc) std::cout << "E mco_metadict_register=" << rc << std::endl;
  }
#endif

  return result;
}

// ===========================================================================
bool db_disconnect()
{
  static const char* fname = "db_disconnect";
  bool result = false;
  MCO_RET rc = MCO_S_OK;

  SaveAsXML();

#if 0
    if (rc) {
      std::cout << "W " << fname << ": db_disconnect '" << m_name << "', rc=" << rc << std::endl;
    }
    else {
      /*
       * Note, mco_stat_collect must be called before
       * mco_calc_iinfo_brouse or mco_calc_cinfo_brouse
       */
      rc = mco_calc_stat_collect(&m_calc);

      if (rc == MCO_S_OK) {

        show_db_stat(&m_calc);

#ifdef VERBOSE
        rc = mco_calc_cinfo_browse(&m_calc, class_iterator, NULL);
        if (rc) {
          std::cout << "W " << fname << ": mco_calc_cinfo_brouse, rc=" << rc << std::endl;
        }
        rc = mco_calc_iinfo_browse(&m_calc, index_iterator, NULL);
        if (rc) {
          std::cout << "W " << fname << ": mco_calc_iinfo_brouse, rc=" << rc << std::endl;
        }
#endif
      }
    }
#endif
  mco_calc_deinit(&m_calc);

  rc = mco_db_disconnect(m_db);
  rc = mco_db_close(m_name);
  rc = mco_runtime_stop();

  if (MCO_S_OK == rc) {
    result = true;
  }
  else {
    std::cout << "W " << fname << ": rc=" << rc << std::endl;
    result = false;
  }
  
  return result;
}

// ===========================================================================
// Заполнить БД тестовой информацией:
// 1. Создать Точку
// 2. Создать массив исторических данных для этой Точки
bool db_fill_data()
{
  bool result = false;
  const char* fname = "db_fill_data";
  const char* pname = HIST_TEST_POINT_NAME;
  const objclass_t point_objclass = TM;
  const HistoryType htype = PER_1_MINUTE;
  int2  slot_number = 100;      // индекс в массиве семплов таблицы HISTORY
  const int quantity = 60 * 24; // 24 часа ежеминутных отсчётов

  result = create_point(pname, point_objclass, slot_number, htype);

  if (true == result) {
    result = create_all_hist(pname, htype, quantity);
  }
  else {
    std::cout << "E " << fname << ": can't create test point " << pname << std::endl;
  }

  return result;
}

ProcessingType get_type_by_objclass(const objclass_t objclass)
{
  const char* fname = "get_type_by_objclass";
  ProcessingType result = UNKNOWN;

  switch(xdb::ClassDescriptionTable[objclass].val_type) {
    case xdb::DB_TYPE_LOGICAL:
      result = BINARY;
      break;

    case xdb::DB_TYPE_UINT16:
      result = DISCRETE;
      break;

    case xdb::DB_TYPE_DOUBLE:
    case xdb::DB_TYPE_FLOAT:
      result = ANALOG;
      break;

    case xdb::DB_TYPE_UNDEF:
    default:
      result = UNKNOWN;
      std::cout << "E " << fname << ": unsupported objclass " << objclass << std::endl;
  }

  return result;
}

// Создать в БД Запись об указанной точке
bool create_point(const char* pname, const objclass_t objclass, int2 slot, const HistoryType htype)
{
  mco_trans_h t;
  bool result = false;
  const char* fname = "create_point";
  ProcessingType ptype;
  MCO_RET rc = MCO_S_OK;
  XDBPoint handle_point;
  Validity validity = VALIDITY_VALID;
  AnalogInfoType ait;

  ptype = get_type_by_objclass(objclass);

  if (UNKNOWN == ptype) {
    return false;
  }

  do {
    rc = mco_trans_start(m_db, MCO_READ_WRITE, MCO_TRANS_FOREGROUND, &t);
    if (rc) { std::cout << "E " << fname << ": start transaction, rc=" << rc << std::endl; break; }

    rc = XDBPoint_new(t, &handle_point);
    if (rc) { std::cout << "E " << fname << ": creating point " << pname << ", rc=" << rc << std::endl; break; }

    rc = XDBPoint_TAG_put(&handle_point, pname, strlen(pname));
    if (rc) { std::cout << "E " << fname << ": assign point tag " << pname << ", rc=" << rc << std::endl; break; }

    rc = XDBPoint_OBJCLASS_put(&handle_point, objclass);
    if (rc) {
      std::cout << "E " << fname << ": set point " << pname << " objclass="
                << objclass <<", rc=" << rc << std::endl;
      break;
    }

    rc = XDBPoint_type_put(&handle_point, htype);
    rc = XDBPoint_SHORTLABEL_put(&handle_point, "tili-tili-testo", 15);
    rc = XDBPoint_DE_TYPE_put(&handle_point, 0);
    rc = XDBPoint_VALIDITY_put(&handle_point, validity);
    
    rc = XDBPoint_ai_write_handle(&handle_point, &ait);
    if (rc) { std::cout << "E " << fname << ": allocating " << pname << " analog part, rc=" << rc << std::endl; break; }
    rc = AnalogInfoType_LINK_HIST_put(&ait, slot);
    if (rc) { std::cout << "E " << fname << ": writing " << pname << ".LINK_HIST, rc=" << rc << std::endl; break; }

    rc = mco_trans_commit(t);
    if (rc) { std::cout << "E " << fname << ": commitment transaction, rc=" << rc << std::endl; break; }

    std::cout << "I Creating '" << pname
              << "' objclass: " << objclass
              << " type: " << ptype
              << " [SUCCESS]" << std::endl;

    result = true;

  } while (false);

  if (rc) {
    std::cout << "I Creating '" << pname
              << "' objclass: " << objclass
              << " type: " << ptype
              << "' [FAILURE]" << std::endl;
    mco_trans_rollback(t);
  }

  return result;
}

bool create_all_hist(const char* pname, const HistoryType htype, const int n_samples)
{
  bool result = false;
  mco_trans_h t;
  ProcessingType ptype;
  MCO_RET rc = MCO_S_OK;
  XDBPoint handle_point;
  AnalogInfoType handle_ait;
  autoid_t  point_aid;
  int last_sample_num;
  objclass_t point_objclass;
  const char* fname = "create_all_hist";
  xdb::datetime_t timer;
  int2 history_index;

  // sec = 946684800 : GMT=[Sat, 01 Jan 2000 00:00:00]; MSK=[Sat, 01 Jan 2000 03:00:00]
  // sec = 946674000 : MSK=[Sat, 01 Jan 2000 00:00:00]
  timer.part[0] = HIST_TEST_PERIOD_START;
  timer.part[1] = 0;

  do {

    rc = mco_trans_start(m_db, MCO_READ_WRITE, MCO_TRANS_FOREGROUND, &t);
    if (rc) { std::cout << "E " << fname << ": start transaction, rc=" << rc << std::endl; break; }

    // Получить идентификатор Точки
    rc = XDBPoint_SK_by_tag_find(t, pname, strlen(pname), &handle_point);
    if (rc) { std::cout << "E " << fname << ": locating point " << pname << ", rc=" << rc << std::endl; break; }

    rc = XDBPoint_autoid_get(&handle_point, &point_aid);
    if (rc) { std::cout << "E " << fname << ": get point " << pname << " id, rc=" << rc << std::endl; break; }

    // Получить objclass Точки
    rc = XDBPoint_OBJCLASS_get(&handle_point, &point_objclass);
    if (rc) { std::cout << "E " << fname << ": get point " << pname << " objclass, rc=" << rc << std::endl; break; }

    // По её objclass найти тип значения - DISCRETE, BINARY, ANALOG
    ptype = get_type_by_objclass(point_objclass);
    if (ANALOG != ptype) break;
    
    // Найти индекс этой Точки в массиве истории
    rc = XDBPoint_ai_read_handle(&handle_point, &handle_ait);
    if (rc) { std::cout << "E " << fname << ": get analog part of " << pname << ", rc=" << rc << std::endl; break; }

    rc = AnalogInfoType_LINK_HIST_get(&handle_ait, &history_index);
    if (rc) { std::cout << "E " << fname << ": read history indexer of " << pname << ", rc=" << rc << std::endl; break; }

    rc = mco_trans_commit(t);

    for (last_sample_num = 0; last_sample_num < n_samples; last_sample_num++) {
      rc = create_hist_item(last_sample_num, htype, timer, point_aid, history_index);

      switch(htype) {
        case PER_1_MINUTE:
          timer.tv.tv_sec += 60;
          break;
        case PER_5_MINUTES:
          timer.tv.tv_sec += (60 * 5);
          break;
        case PER_HOUR:
          timer.tv.tv_sec += (60 * 60);
          break;
        case PER_DAY:
          timer.tv.tv_sec += (60 * 60 * 24);
          break;
        default:
          std::cout << "E " << fname << ": unsupported time interval: " << htype << std::endl;
          rc = MCO_E_UNSUPPORTED;
      }

      if (rc) { std::cout << "E " << fname << ": storing sample item " << last_sample_num << "/" << n_samples << std::endl; break; }
    }

  } while (false);

  std::cout << "I " << fname << ": created " << last_sample_num << "/" << n_samples
            << " history records"
            << " of '" << pname << "'" << std::endl;

  if (MCO_S_OK == rc) {
    result = true;
  }
  else {
    mco_trans_rollback(t);
  }

  return result;
}

// Создать набор записей для требуемой Точки в соответствующее место
// (hist_index) массива истории.
MCO_RET create_hist_item(int cur_sample,
                         HistoryType htype,
                         xdb::datetime_t given_timer,
                         autoid_t point_aid,
                         int2 slot_num)
{
  MCO_RET rc;
  HISTORY_1_MIN handle_1_min_hist;
  HISTORY_5_MIN handle_5_min_hist;
  AnalogHistoryInfo ahi;
  Validity validity = VALIDITY_VALID;
  autoid_t hist_item_aid = 0;
  timestamp timer;
//  bool optional_created = false;
  static const char* fname = "create_hist_item";
  double   val = 0.0;
  // Будет ошибка, если слишком много объектов создается внутри одной транзакции
  mco_trans_h t;

  do {
    rc = mco_trans_start(m_db, MCO_READ_WRITE, MCO_TRANS_FOREGROUND, &t);
    if (rc) { std::cout << "E " << fname << ": start transaction, rc=" << rc << std::endl; break; }
 
    if (PER_1_MINUTE == htype)       rc = HISTORY_1_MIN_new(t, &handle_1_min_hist);
    else if (PER_5_MINUTES == htype) rc = HISTORY_5_MIN_new(t, &handle_5_min_hist);
    else rc = MCO_E_UNSUPPORTED;
    if (rc) {
      std::cout << "E " << fname << ": creating history item type="
                << htype << ", rc=" << rc << std::endl;
      break;
    }

    if (PER_1_MINUTE == htype)       rc = HISTORY_1_MIN_autoid_get(&handle_1_min_hist, &hist_item_aid);
    else if (PER_5_MINUTES == htype) rc = HISTORY_5_MIN_autoid_get(&handle_5_min_hist, &hist_item_aid);
    else rc = MCO_E_UNSUPPORTED;
    if (rc) {
      std::cout << "E " << fname << ": getting new history item type="
                << htype << ", rc=" << rc << std::endl;
      break;
    }

    if (PER_1_MINUTE == htype)       rc = HISTORY_1_MIN_ahi_at(&handle_1_min_hist, slot_num, &ahi);
    else if (PER_5_MINUTES == htype) rc = HISTORY_5_MIN_ahi_at(&handle_5_min_hist, slot_num, &ahi);
    else rc = MCO_E_UNSUPPORTED;
    if (rc) {
      std::cout << "E " << fname << ": getting slot " << slot_num
                << " from history type=" << htype
                << ", rc=" << rc << std::endl;
      break;
    }

    val = sin(cur_sample) * 10 + 11;

    rc = AnalogHistoryInfo_VAL_put(&ahi, val);
    if (rc) { std::cout << "E " << fname << std::endl; break; }
    rc = AnalogHistoryInfo_VALIDITY_put(&ahi, validity);
    if (rc) { std::cout << "E " << fname << std::endl; break; }

    if (PER_1_MINUTE == htype)       rc = HISTORY_1_MIN_mark_write_handle(&handle_1_min_hist, &timer);
    else if (PER_5_MINUTES == htype) rc = HISTORY_5_MIN_mark_write_handle(&handle_5_min_hist, &timer);
    else rc = MCO_E_UNSUPPORTED;
    if (rc) { std::cout << "E " << fname << ": allocating datetime, rc=" << rc << std::endl; break; }

    rc = timestamp_sec_put(&timer, given_timer.part[0]);
    rc = timestamp_nsec_put(&timer, 0);

    rc = mco_trans_commit(t);
    if (rc) {
      std::cout << "E " << fname << ": commitment history item id=" << hist_item_aid
                << ", rc=" << rc << std::endl;
      break;
    }

  } while (false);

  if (rc) {
    std::cout << "E " << fname
              << ": can't create new history item type " << htype
              << " for point id " << point_aid << ", slot " << slot_num
              << std::endl;
    mco_trans_rollback(t);
  }
  else {
    std::cout << "I " << fname << ": store " << cur_sample
              << " htype=" << htype
              << " value=" << val
              << " timer=" << given_timer.tv.tv_sec
              << " history item point id=" << point_aid
              << " into slot=" << slot_num
              << std::endl;
  }

  return rc;
}


// ===========================================================================
// Найти последовательность исторических семплов указанного типа (htype)
// для точки с указанным именем (pname), начинающуюся с момента времени
// (start) и состоящую из указанного количества (n_samples)
bool locate_period(const char* pname, time_t start, const HistoryType htype, const int n_samples)
{
  bool status = true;
  const char* fname = "locate_period";
  time_t frame = start;
  XDBPoint handle_point;
  autoid_t point_aid;
  mco_trans_h t;
  time_t delta;
  AnalogInfoType ai;
  int2 slot_idx; // индекс слота в массиве истории для данной Точки
  MCO_RET rc;

  switch (htype) {
    case PER_1_MINUTE:  delta = 60 * 1;  break;
    case PER_5_MINUTES: delta = 60 * 5;  break;
    case PER_HOUR:      delta = 60 * 60; break;
    case PER_DAY:       delta = 60 * 60 * 24; break;
    case PER_MONTH:     delta = 60 * 60 * 24 * 31; break;
    default:
      std::cout << "E " << fname << ": unsupported history type: " << htype << std::endl;
      return false;
  }

  do {
    // В таблице XDBPoint найти идентификатор тега
    rc = mco_trans_start(m_db, MCO_READ_ONLY, MCO_TRANS_FOREGROUND, &t);
    if (rc) { std::cout << "E " << fname << ": start transaction, rc=" << rc << std::endl; break; }

    // Получить идентификатор Точки
    rc = XDBPoint_SK_by_tag_find(t, pname, strlen(pname), &handle_point);
    if (rc) { std::cout << "E " << fname << ": locating point " << pname << ", rc=" << rc << std::endl; break; }

    rc = XDBPoint_autoid_get(&handle_point, &point_aid);
    if (rc) { std::cout << "E " << fname << ": get point " << pname << " id, rc=" << rc << std::endl; break; }

    // Узнать нужный индекс массива истории
    rc = XDBPoint_ai_read_handle(&handle_point, &ai);
    if (rc) { std::cout << "E " << fname << ": get " << pname << " AnalogInfoType, rc=" << rc << std::endl; break; }

    rc = AnalogInfoType_LINK_HIST_get(&ai, &slot_idx);
    if (rc) { std::cout << "E " << fname << ": get " << pname << " history slot idx, rc=" << rc << std::endl; break; }

    rc = mco_trans_commit(t);

  } while (false);

  if (rc) {
    std::cout << "E " << fname << ": locating historized point '"
              << pname << "' info, rc=" << rc << std::endl;
    mco_trans_rollback(t);
  }
  else {
    // Все по Точке в XDBPoint нашли, теперь залезем в таблицу HISTORY
    for (frame = start; frame < (start + delta * n_samples); frame += delta) {

      // TODO: прочитать значение семпла на этот период
      status = locate_samples(point_aid, slot_idx, frame, htype);

      if (!status) {
        std::cout << "W " << fname << ": can't read '" << pname << "' sample "
                  << htype << " period for "
                  << frame << " sec "
                  << std::endl;
        // TODO: Пропускать сбойный отсчёт?
      }
    }
  }

  return status;
}

// ===========================================================================
// Найти последовательность исторических семплов указанного типа (htype)
// для точки с указанным именем (pname), начинающуюся с момента времени
// (start), хранящуюся в массиве истории под указанным индексом (slot_idx)
// и состоящую из указанного количества (n_samples)
bool locate_samples(autoid_t point_aid, int2 slot_idx, time_t start, const HistoryType htype)
{
  const char* fname = "locate_samples";
  bool status = false;
  HISTORY_1_MIN handle_1_min_history;
  HISTORY_5_MIN handle_5_min_history;
//  HistoryType history_type;
  AnalogHistoryInfo ahi;
  autoid_t history_point_aid;
  mco_trans_h t;
  mco_cursor_t csr;
  MCO_RET rc;
  int compare_result = 0;
  // Значение и Достоверность одного семпла
  double value;
  Validity valid;

  do {
    // В таблице HISTORY найти записи, соответствующие найденному идентификатору и времени начала
    rc = mco_trans_start(m_db, MCO_READ_ONLY, MCO_TRANS_FOREGROUND, &t);
    if (rc) { std::cout << "E " << fname << ": start transaction, rc=" << rc << std::endl; break; }

    if (PER_1_MINUTE == htype)       rc = HISTORY_1_MIN_SK_by_time_index_cursor(t, &csr);
    else if (PER_5_MINUTES == htype) rc = HISTORY_5_MIN_SK_by_time_index_cursor(t, &csr);
    else rc = MCO_E_UNSUPPORTED;
    if (rc) { std::cout << "E " << fname << ": get SK_by_point_time cursor, rc=" << rc << std::endl; break; }

    if (PER_1_MINUTE == htype)       rc = HISTORY_1_MIN_SK_by_time_search(t, &csr, MCO_EQ, start);
    else if (PER_5_MINUTES == htype) rc = HISTORY_5_MIN_SK_by_time_search(t, &csr, MCO_EQ, start);
    else rc = MCO_E_UNSUPPORTED;
    if (rc) {
      // Ошибка?
      if (rc == MCO_S_NOTFOUND) {
        // Не найдены искомые записи - плохо, но бывает...
        std::cout << "W " << fname << ": empty SK_by_time cursor for point id="
                  << point_aid << " and " << start << " (sec)" << std::endl;
      }
      else {
        // Да, ошибка
        std::cout << "E " << fname << ": SK_by_time cursor for point id="
                  << point_aid << ", " << start << " (sec), rc=" << rc << std::endl;
      }
      break;
    }

    while (MCO_S_OK == rc) {
      if (PER_1_MINUTE == htype)       rc = HISTORY_1_MIN_SK_by_time_compare(t, &csr, start, &compare_result);
      else if (PER_5_MINUTES == htype) rc = HISTORY_5_MIN_SK_by_time_compare(t, &csr, start, &compare_result);
      else rc = MCO_E_UNSUPPORTED;
      if (rc) {
        std::cout << "E " << fname << ": Unable to compare with id="<<point_aid<<", rc=" << rc << std::endl;
        break;
      }

      if (0 == compare_result) { // Найден элемент с указаным временем
        // Проверить, ссылается ли найденный элемент на указанный идентификатор Точки?
        if (PER_1_MINUTE == htype)       rc = HISTORY_1_MIN_from_cursor(t, &csr, &handle_1_min_history);
        else if (PER_5_MINUTES == htype) rc = HISTORY_5_MIN_from_cursor(t, &csr, &handle_5_min_history);
        else rc = MCO_E_UNSUPPORTED;
        if (rc) {
          std::cout << "E " << fname
                    << ": Unable to locate item from SK_by_time cursor with id="
                    << point_aid << ", rc=" << rc << std::endl;
          break;
        }

        if (PER_1_MINUTE == htype)       rc = HISTORY_1_MIN_ahi_at(&handle_1_min_history, slot_idx, &ahi);
        else if (PER_5_MINUTES == htype) rc = HISTORY_5_MIN_ahi_at(&handle_5_min_history, slot_idx, &ahi);
        if (rc) {
          std::cout << "E " << fname
                    << ": Unable to get history item idx=" << slot_idx
                    << " type=" << htype
                    << " for point id=" << point_aid
                    << ", rc=" << rc
                    << std::endl;
          break;
        }
        else {
          rc = AnalogHistoryInfo_VAL_get(&ahi, &value);
          if (rc) { std::cout << "E " << fname << " get VAL for history idx=" << slot_idx << std::endl; break; };

          rc = AnalogHistoryInfo_VALIDITY_get(&ahi, &valid);
          if (rc) { std::cout << "E " << fname << " get VALIDITY for history idx=" << slot_idx << std::endl; break; };

          std::cout << "Gotcha! Point id=" << point_aid
                    << ", type=" << htype
                    << ", val=" << value
                    << ", valid=" << valid
                    << ", slot=" << slot_idx
                    << ", " << start << " (sec)"
                    << std::endl;

          status = true;
          // NB: Найдена Точка с требуемым идентификатором, типом истории, и датой.
          // Более таких быть не должно, можно прервать поиск.
          break;
        }

        rc = mco_cursor_next(t, &csr);
      }
      else {
        // Пропустим этот элемент
        rc = mco_cursor_next(t, &csr);
      }
    }

    rc = mco_cursor_close(t, &csr);
    if (rc) { std::cout << "E " << fname << ": closing SK_by_time cursor, rc=" << rc << std::endl; break; }








    rc = mco_trans_commit(t);

  } while (false);

  if (rc)
    mco_trans_rollback(t);

  return status;
}


// ===========================================================================
int main(int argc, char* argv[])
{
  bool status = db_connect();

  if (true == status) {

    if (true == (db_fill_data())) {
      std::cout << "I main: DB updated" << std::endl;
    }
    else {
      std::cout << "F main: can't update DB" << std::endl;
    }

    if (true == (status = locate_period(HIST_TEST_POINT_NAME,
                                        HIST_TEST_PERIOD_START,
                                        PER_1_MINUTE,
                                        60))) {

      std::cout << "I main: reading samples [SUCCESS]" << std::endl;
    }
    else {
      std::cout << "I main: reading samples [FAILURE]" << std::endl;
    }

    db_disconnect();
  }
  else {
    std::cout << "F main: can't attach to DB" << std::endl;
  }

  return 0;
}

