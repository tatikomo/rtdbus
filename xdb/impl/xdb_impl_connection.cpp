#include <assert.h>

#if defined HAVE_CONFIG_H
#include "config.h"
#endif

#include "glog/logging.h"

#ifdef __cplusplus
extern "C" {
#include "mco.h"
}
#endif

#include "timer.hpp"

#include "xdb_impl_connection.hpp"
#include "xdb_impl_environment.hpp"

#include "dat/rtap_db.h"
#include "dat/rtap_db.hpp"
#include "dat/rtap_db.hxx"

using namespace xdb;

ConnectionImpl::ConnectionImpl(EnvironmentImpl *env)
 : m_env_impl(env),
   m_database_handle(NULL)
{
  MCO_RET rc;

  rc = mco_db_connect(m_env_impl->getName(), &m_database_handle);
  if (rc)
  {
    LOG(ERROR) << "Couldn't get new database '"<< m_env_impl->getName() <<"' connection, rc="<<rc;
  }
  else LOG(INFO) << "Get new database '"<< m_env_impl->getName() <<"' connection " << m_database_handle;
}

ConnectionImpl::~ConnectionImpl()
{
  MCO_RET rc;

  LOG(INFO) << "Closing database '"<<m_env_impl->getName()<<"' connection " << m_database_handle;
  if (m_database_handle)
  {
    rc = mco_db_disconnect(m_database_handle);
    if (rc) {
        LOG(ERROR) << "Closing database '" << m_env_impl->getName() 
        << "' connection failure, rc=" << rc; }
  }
}

mco_db_h ConnectionImpl::handle()
{
  return m_database_handle;
}

// Найти точку с указанным тегом
rtap_db::Point* ConnectionImpl::locate(const char* _tag)
{
  mco_trans_h t;
  MCO_RET rc = MCO_S_OK;
  rtap_db::XDBPoint instance;
  uint2 tag_size;
  uint4 timer_value;
  timer_mark_t now_time = {0, 0};
  timer_mark_t prev_time = {0, 0};
  rtap_db::timestamp datehourm;

  assert(_tag);

  LOG(INFO) << "Locating point " << _tag;
  tag_size = strlen(_tag);
      
  do
  {
    rc = mco_trans_start(m_database_handle, MCO_READ_WRITE, MCO_TRANS_FOREGROUND, &t);
    if (rc) { LOG(ERROR) << "Starting transaction, rc=" << rc; break; }

    rc = rtap_db::XDBPoint::SK_by_tag::find(t, _tag, tag_size, instance);
    if (rc) { LOG(ERROR) << "Locating point '" << _tag << "', rc=" << rc; break; }

    rc = instance.DATEHOURM_read(datehourm);
    if (rc) { LOG(ERROR) << "Reading DATEHOURM, point '" << _tag << "', rc=" << rc; break; }

    datehourm.sec_get(timer_value);  prev_time.tv_sec  = timer_value;
    datehourm.nsec_get(timer_value); prev_time.tv_nsec = timer_value;

    GetTimerValue(now_time);

    LOG(INFO) << "Difference between DATEHOURM : " << now_time.tv_sec - prev_time.tv_sec
              << "." << (now_time.tv_nsec - prev_time.tv_nsec) / 1000;

    datehourm.sec_put(now_time.tv_sec);
    datehourm.nsec_put(now_time.tv_nsec);

    rc = instance.DATEHOURM_write(datehourm);
    if (rc) { LOG(ERROR) << "Writing DATEHOURM, point '" << _tag << "', rc=" << rc; break; }

    rc = mco_trans_commit(t);

  } while (false);

  if (rc)
    mco_trans_rollback(t);

  return NULL;
}


