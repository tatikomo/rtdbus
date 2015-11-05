#if defined HAVE_CONFIG_H
#include "config.h"
#endif

#include <assert.h>

#include <sys/syscall.h> // gettid()
#include <sys/time.h>
#include <sys/resource.h>

#include <iostream>
#include <vector>
#include <limits>

#include <stdlib.h>
#include <sys/types.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include "tool_metrics.hpp"

using namespace tool;

UsageRow::UsageRow() :
  m_cpu_usage_idx(0),
  m_ucpu_usage_pct(0),
  m_ucpu_usage(0),
  m_scpu_usage_pct(0),
  m_scpu_usage(0),
  // Заполнить неоправданно большими значениями для того, чтобы первое
  // добавляемое измерение стало новым минимумом вместо нулей по умолчанию.
  m_pstat_min ({ BILLION,
                std::numeric_limits<double>::max(),
                std::numeric_limits<double>::max(),
                std::numeric_limits<double>::max(),
                std::numeric_limits<double>::max(),
                std::numeric_limits<double>::max(),
                std::numeric_limits<double>::max()
               }),
  m_pstat_avg ({ 0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0 }),
  m_pstat_max ({ 0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0 })
{
  memset(&m_cpu_usage, '\0', sizeof(thread_stat_t) * MAX_NUM_SAMPLES);
}

UsageRow::~UsageRow()
{
}

// Добавить новый отсчет
void UsageRow::add(const thread_stat_t& _item)
{
  thread_stat_t average = { 0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0 };

#if 1
  assert(_item.ru_rss >= 0);
  assert(_item.between_2_requests >= 0);
  assert(_item.processing_duration >= 0);
  assert(_item.ucpu_usage_pct >= 0);
  assert(_item.ucpu_usage >= 0);
  assert(_item.scpu_usage_pct >= 0);
  assert(_item.scpu_usage >= 0);
#endif

  m_cpu_usage[m_cpu_usage_idx++] = _item;
  if (m_cpu_usage_idx >= MAX_NUM_SAMPLES)
    m_cpu_usage_idx = 0;

  ////////////////////////////////////////////////////////////////
  // Проверить на минимальность и максимальность
  ////////////////////////////////////////////////////////////////

  // 1. анализ поля ru_rss
  // =================================
  if (_item.ru_rss < m_pstat_min.ru_rss)
    m_pstat_min.ru_rss = _item.ru_rss;

  if (_item.ru_rss > m_pstat_max.ru_rss)
    m_pstat_max.ru_rss = _item.ru_rss;
  
  // 2. анализ поля between_2_requests
  // =================================
  if (_item.between_2_requests < m_pstat_min.between_2_requests)
    m_pstat_min.between_2_requests = _item.between_2_requests;

  if (_item.between_2_requests > m_pstat_max.between_2_requests)
    m_pstat_max.between_2_requests = _item.between_2_requests;

  // 3. анализ поля ucpu_usage_pct
  // =================================
  if (_item.ucpu_usage_pct < m_pstat_min.ucpu_usage_pct)
    m_pstat_min.ucpu_usage_pct = _item.ucpu_usage_pct;

  if (_item.ucpu_usage_pct > m_pstat_max.ucpu_usage_pct)
    m_pstat_max.ucpu_usage_pct = _item.ucpu_usage_pct;
  
  // 4. анализ поля ucpu_usage
  // =================================
  if (_item.ucpu_usage < m_pstat_min.ucpu_usage)
    m_pstat_min.ucpu_usage = _item.ucpu_usage;

  if (_item.ucpu_usage > m_pstat_max.ucpu_usage)
    m_pstat_max.ucpu_usage = _item.ucpu_usage;

  // 5. анализ поля scpu_usage_pct
  // =================================
  if (_item.scpu_usage_pct < m_pstat_min.scpu_usage_pct)
    m_pstat_min.scpu_usage_pct = _item.scpu_usage_pct;

  if (_item.scpu_usage_pct > m_pstat_max.scpu_usage_pct)
    m_pstat_max.scpu_usage_pct = _item.scpu_usage_pct;

  // 6. анализ поля scpu_usage
  // =================================
  if (_item.scpu_usage < m_pstat_min.scpu_usage)
    m_pstat_min.scpu_usage = _item.scpu_usage;

  if (_item.scpu_usage > m_pstat_max.scpu_usage)
    m_pstat_max.scpu_usage = _item.scpu_usage;

  // 7. анализ поля between_2_requests
  // =================================
  if (_item.processing_duration < m_pstat_min.processing_duration)
    m_pstat_min.processing_duration = _item.processing_duration;

  if (_item.processing_duration > m_pstat_max.processing_duration)
    m_pstat_max.processing_duration = _item.processing_duration;

  ////////////////////////////////////////////////////////////////
  // Найти среднее
  ////////////////////////////////////////////////////////////////
  for (unsigned int idx = 0; idx < MAX_NUM_SAMPLES; idx++)
  {
    average.ru_rss += m_cpu_usage[idx].ru_rss;
//    average.ru_rss /= (m_cpu_usage[idx].ru_rss == 0)? 1 : 2;

    average.between_2_requests += m_cpu_usage[idx].between_2_requests;
//    average.between_2_requests /= (m_cpu_usage[idx].between_2_requests == 0)? 1 : 2;

    average.processing_duration += m_cpu_usage[idx].processing_duration;
//    average.processing_duration /= (m_cpu_usage[idx].processing_duration == 0)? 1 : 2;

    average.ucpu_usage_pct += m_cpu_usage[idx].ucpu_usage_pct;
//    average.ucpu_usage_pct /= (m_cpu_usage[idx].ucpu_usage_pct == 0)? 1 : 2;

    average.ucpu_usage += m_cpu_usage[idx].ucpu_usage;
//    average.ucpu_usage /= (m_cpu_usage[idx].ucpu_usage == 0)? 1 : 2;

    average.scpu_usage_pct += m_cpu_usage[idx].scpu_usage_pct;
//    average.scpu_usage_pct /= (m_cpu_usage[idx].scpu_usage_pct == 0)? 1 : 2;

    average.scpu_usage += m_cpu_usage[idx].scpu_usage;
//    average.scpu_usage /= (m_cpu_usage[idx].scpu_usage == 0)? 1 : 2;
  }

  // Получить среднее арифметическое
  average.ru_rss /= MAX_NUM_SAMPLES;
  average.between_2_requests /= MAX_NUM_SAMPLES;
  average.processing_duration /= MAX_NUM_SAMPLES;
  average.ucpu_usage_pct /= MAX_NUM_SAMPLES;
  average.ucpu_usage /= MAX_NUM_SAMPLES;
  average.scpu_usage_pct /= MAX_NUM_SAMPLES;
  average.scpu_usage /= MAX_NUM_SAMPLES;

  // Перенести подсчитанное
  m_pstat_avg.ru_rss = average.ru_rss;
  m_pstat_avg.between_2_requests = average.between_2_requests;
  m_pstat_avg.processing_duration = average.processing_duration;
  m_pstat_avg.ucpu_usage_pct = average.ucpu_usage_pct;
  m_pstat_avg.ucpu_usage = average.ucpu_usage;
  m_pstat_avg.scpu_usage_pct = average.scpu_usage_pct;
  m_pstat_avg.scpu_usage = average.scpu_usage;

  std::cout << "pid:"       << static_cast<long int>(syscall(SYS_gettid))
            << "\tidx:"     << m_cpu_usage_idx << std::endl;
  std::cout << "in\trss:"   << _item.ru_rss
            << "\tb2r:"     << _item.between_2_requests
            << "\tpd:"      << _item.processing_duration
            << "\tu%:"      << _item.ucpu_usage_pct
            << "\tu#:"      << _item.ucpu_usage
            << "\ts%:"      << _item.scpu_usage_pct
            << "\ts#:"      << _item.scpu_usage
            << std::endl;

#if defined VERBOSE
  std::cout << "min\trss:"  << m_pstat_min.ru_rss
            << "\tb2r:"     << m_pstat_min.between_2_requests
            << "\tpd:"      << m_pstat_min.processing_duration
            << "\tu%:"      << m_pstat_min.ucpu_usage_pct
            << "\tu#:"      << m_pstat_min.ucpu_usage
            << "\ts%:"      << m_pstat_min.scpu_usage_pct
            << "\ts#:"      << m_pstat_min.scpu_usage
            << std::endl;
#endif
  
  std::cout << "avg\trss:"  << m_pstat_avg.ru_rss
            << "\tb2r:"     << m_pstat_avg.between_2_requests
            << "\tpd:"      << m_pstat_avg.processing_duration
            << "\tu%:"      << m_pstat_avg.ucpu_usage_pct
            << "\tu#:"      << m_pstat_avg.ucpu_usage
            << "\ts%:"      << m_pstat_avg.scpu_usage_pct
            << "\ts#:"      << m_pstat_avg.scpu_usage
            << std::endl;

#if defined VERBOSE
  std::cout << "max\trss:"  << m_pstat_max.ru_rss
            << "\tb2r:"     << m_pstat_max.between_2_requests
            << "\tpd:"      << m_pstat_max.processing_duration
            << "\tu%:"      << m_pstat_max.ucpu_usage_pct
            << "\tu#:"      << m_pstat_max.ucpu_usage
            << "\ts%:"      << m_pstat_max.scpu_usage_pct
            << "\ts#:"      << m_pstat_max.scpu_usage
            << std::endl;
#endif
}

// Вернуть накопленное минимальное значение
const thread_stat_t& UsageRow::min() const
{
  return m_pstat_min;
}

// Вернуть накопленное среднее значение
const thread_stat_t& UsageRow::average() const
{
  return m_pstat_avg;
}

// Вернуть накопленное максимальное значение
const thread_stat_t& UsageRow::max() const
{
  return m_pstat_max;
}

Metrics::Metrics() :
  m_pid(getpid()),
  m_time_before({0, 0}),
  m_time_after({0, 0}),
  m_usage_metric(new UsageRow()),
  m_pstat_before(),
  m_pstat_after(),
  m_thread_stat({ 0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0 })
{
  memset(&m_pstat_before, '\0', sizeof(m_pstat_before));
  memset(&m_pstat_after, '\0', sizeof(m_pstat_after));
//  m_thread_stat = { 0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0 };
}

Metrics::~Metrics()
{
  delete m_usage_metric;
}

void Metrics::after()
{
#if 0
  char buf0[100];
  char buf1[100];
#endif
  struct timeval diff_sys;
  struct timeval diff_usr;

  // Поле m_time_after пока еще содержит время с момента окончания предыдущего запроса.
  // Следует вычесть это значение из времени начала текущего запроса для получения 
  // интервала между двумя запросами.
  if (m_time_after.tv_sec && m_time_after.tv_nsec)
    m_thread_stat.between_2_requests = timeSeconds(timeDiff(m_time_after, m_time_before));
  else // это первое событие после запуска
    m_thread_stat.between_2_requests = 0;

  GetTimerValueByClockId(CLOCK_MONOTONIC/*CLOCK_THREAD_CPUTIME_ID*/, m_time_after);
  getrusage(RUSAGE_THREAD, &m_pstat_after);

#if 0
  sprintf(buf0, "%ld.%.9ld", m_time_before.tv_sec, m_time_before.tv_nsec);
  sprintf(buf1, "%ld.%.9ld", m_time_after.tv_sec, m_time_after.tv_nsec);
  std::cout << "clock:\t" << buf0 << "\t" << buf1 << std::endl;

  sprintf(buf0, "%ld.%.6ld", m_pstat_before.ru_utime.tv_sec, m_pstat_before.ru_utime.tv_usec);
  sprintf(buf1, "%ld.%.6ld", m_pstat_after.ru_utime.tv_sec, m_pstat_after.ru_utime.tv_usec);
  std::cout << "utime:\t" << buf0 << "\t" << buf1 << std::endl;

  sprintf(buf0, "%ld.%.6ld", m_pstat_before.ru_stime.tv_sec, m_pstat_before.ru_stime.tv_usec);
  sprintf(buf1, "%ld.%.6ld", m_pstat_after.ru_stime.tv_sec, m_pstat_after.ru_stime.tv_usec);
  std::cout << "stime:\t" << buf0 << "\t" << buf1 << std::endl;
#endif

  // Получить разницу в долях секунд между событиями (наносекунды)
  m_thread_stat.processing_duration = timeSeconds(timeDiff(m_time_before, m_time_after));

  // Получить в diff_usr разницу времени в режиме USR между началом и концом (милисекунды)
  timersub(&m_pstat_after.ru_utime, &m_pstat_before.ru_utime, &diff_usr);
  //std::cout << "diff_usr=" << diff_usr.tv_sec << "." << diff_usr.tv_usec << std::endl;
  // Получить в diff_sys разницу времени в режиме SYS между началом и концом
  timersub(&m_pstat_after.ru_stime, &m_pstat_before.ru_stime, &diff_sys);
  //std::cout << "diff_sys=" << diff_sys.tv_sec << "." << diff_sys.tv_usec << std::endl;

  m_thread_stat.ucpu_usage_pct =
    ((double)diff_usr.tv_sec + ((double)diff_usr.tv_usec / 1000000.0)) / m_thread_stat.processing_duration;
  m_thread_stat.scpu_usage_pct =
    ((double)diff_sys.tv_sec + ((double)diff_sys.tv_usec / 1000000.0)) / m_thread_stat.processing_duration;

  // Получить в diff_usr разницу времени в режиме USR между началом и концом
  timersub(&m_pstat_after.ru_utime, &m_pstat_before.ru_utime, &diff_usr);
  //std::cout << "diff_usr=" << diff_usr.tv_sec << "." << diff_usr.tv_usec << std::endl;
  // Получить в diff_sys разницу времени в режиме SYS между началом и концом
  timersub(&m_pstat_after.ru_stime, &m_pstat_before.ru_stime, &diff_sys);
  //std::cout << "diff_sys=" << diff_sys.tv_sec << "." << diff_sys.tv_usec << std::endl;

  m_thread_stat.ucpu_usage = (double)diff_usr.tv_sec + ((double)diff_usr.tv_usec / 1000000.0);
  m_thread_stat.scpu_usage = (double)diff_sys.tv_sec + ((double)diff_sys.tv_usec / 1000000.0);

  m_thread_stat.ru_rss = m_pstat_after.ru_maxrss;

  m_usage_metric->add(m_thread_stat);
}

void Metrics::before()
{
  GetTimerValueByClockId(CLOCK_MONOTONIC/*CLOCK_THREAD_CPUTIME_ID*/, m_time_before);
  getrusage(RUSAGE_THREAD, &m_pstat_before);
}

void Metrics::statistic()
{
  std::cout << "stat b2r=" << m_thread_stat.between_2_requests
            << "\twork="   << m_thread_stat.processing_duration
            << "\tusr(%)=" << m_thread_stat.ucpu_usage_pct
            << "\tusr(#)=" << m_thread_stat.ucpu_usage
            << "\tsys(%)=" << m_thread_stat.scpu_usage_pct
            << "\tsys(#)=" << m_thread_stat.scpu_usage
            << std::endl;
}

