#include "xdb_database_letter.hpp"

Letter::Letter(int64_t _service_id, int64_t _worker_id)
{
  m_service_id = _service_id;
  m_worker_id = _worker_id;
}

const int64_t Letter::GetWORKER_ID()
{
  return m_worker_id;
}

const int64_t Letter::GetSERVICE_ID()
{
  return m_service_id;
}
