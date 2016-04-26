#pragma once
#ifndef TOOLS_METRICS_HPP
#define TOOLS_METRICS_HPP

#include <sys/time.h>
#include <sys/resource.h>

#include <sys/types.h>
#include <unistd.h>

#if defined HAVE_CONFIG_H
#include "config.h"
#endif
#include "tool_time.hpp"

// Количество рядов хранения
#define MAX_NUM_SAMPLES 5

namespace tool
{

// Структура элементарного хранения элемента производительности
typedef struct
{
  long   ru_rss;        /* resident set size */
  // Время ожидания между двумя запросами (в сек.нсек)
  double between_2_requests;
  // Время обработки запроса (в сек.нсек)
  double processing_duration;
  // Процент использования в пользовательском режиме
  double ucpu_usage_pct;
  // Время в секундах и милисекундах в пользовательском режиме
  double ucpu_usage;
  // Процент использования в режиме ядра
  double scpu_usage_pct;
  // Время в секундах и милисекундах в режиме ядра
  double scpu_usage;
} thread_stat_t;

class UsageRow
{
  public:
    UsageRow();
   ~UsageRow();

    // Добавить новый отсчет
    void add(const thread_stat_t&);
    // Вернуть накопленное минимальное значение
    const thread_stat_t& min() const;
    // Вернуть накопленное среднее значение
    const thread_stat_t& average() const;
    // Вернуть накопленное максимальное значение
    const thread_stat_t& max() const;

  private:
    // Хранилище отсчетов
    thread_stat_t m_cpu_usage[MAX_NUM_SAMPLES];
    // Индекс текущей позиции
    unsigned int m_cpu_usage_idx;
    // Процент использования в пользовательском режиме
    double m_ucpu_usage_pct;
    // Время в секундах и милисекундах в пользовательском режиме
    double m_ucpu_usage;
    // Процент использования в режиме ядра
    double m_scpu_usage_pct;
    // Время в секундах и милисекундах в режиме ядра
    double m_scpu_usage;

    thread_stat_t m_pstat_min;
    thread_stat_t m_pstat_avg;
    thread_stat_t m_pstat_max;
};

class Metrics
{
  public:
    // Создать сервис, наблюдающий за процессом, заданным своим pid
    Metrics();
   ~Metrics();
    void add_thread(pid_t);

    void before();
    void after();
    void statistic();

    // Вернуть накопленное минимальное значение
    const thread_stat_t& min() const { return m_usage_metric->min(); };
    // Вернуть накопленное среднее значение
    const thread_stat_t& average() const { return m_usage_metric->average(); };
    // Вернуть накопленное максимальное значение
    const thread_stat_t& max() const { return m_usage_metric->max(); };

  private:
    DISALLOW_COPY_AND_ASSIGN(Metrics);
    // Идентификатор процесса
    pid_t   m_pid;
    // Идентификатор нити
    //pid_t   m_tid;
    timer_mark_t m_time_before;
    timer_mark_t m_time_after;
    // Отметки производительности
    UsageRow* m_usage_metric;
    
    struct rusage m_pstat_before;
    struct rusage m_pstat_after;

    thread_stat_t m_thread_stat;
};

} // namespace tool

#endif

