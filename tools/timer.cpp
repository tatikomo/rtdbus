#include <time.h>
#include <errno.h>
#include <stdio.h>
#include "config.h"
#include "timer.hpp"

int GetTimerValue(timer_mark_t& _timer)
{
  return GetTimerValueByClockId(CLOCK_TYPE, _timer);
}

int GetTimerValueByClockId(clockid_t id, timer_mark_t& timer)
{
  struct timespec res;
  int status = 0;

  if (clock_gettime(id, &res) != -1)
  {
    timer.tv_sec = res.tv_sec;
    timer.tv_nsec = res.tv_nsec;
    status = 1;
  }
  else
  {
    perror("GetTimerValue");
  }

  return status;
}

// =================================================================================
// Вернуть количество секунд, оставшихся до границы минуты
// NB: Используется монотонный таймер, не подверженный броскам времени NTP
long get_monotonic_seconds_to_minute_edge()
{
  // Текущее время, используется для ожидания ровно до начала минуты
  // Монотонно возрастающее значение, не совпадающее с часами реального времени
  timer_mark_t time_current_monotonic;
  timer_mark_t time_edge;
  long sec_to_edge_monotonic;
  long s1, s2, s3;

  // дождаться начала цикла (00 секунд?)
  time_current_monotonic = getTime();
  s1 = time_current_monotonic.tv_sec / 60;
  s2 = s1 * 60;
  // округлить до ближайшей границы минуты
  s3 = s2 + 60;

  time_edge.tv_sec = s3;
  //time_edge.tv_nsec = 1000000000 - time_current_monotonic.tv_nsec - 1;
/*
  std::cout << time_current_monotonic.tv_sec << "\t"
            << "s1=" << s1 << "\t"
            << "s2=" << s2 << "\t"
            << "s3=" << s3 << "\t"
            << std::endl;
*/
  sec_to_edge_monotonic = time_edge.tv_sec - time_current_monotonic.tv_sec;
  if (60 == sec_to_edge_monotonic) sec_to_edge_monotonic = 0;
  //std::cout << "sec_to_edge_monotonic=" << sec_to_edge_monotonic << std::endl;

  return sec_to_edge_monotonic;
}

// =================================================================================
// Вернуть количество секунд, оставшихся до границы минуты
// NB: Используются часы реалтайм, подверженные броскам времени NTP
long get_realtime_seconds_to_minute_edge()
{
  // Текущее время, подверженное влиянию NTP. Не гарантируется монотонность
  timer_mark_t time_current_realtime;
  long seconds_to_minute_edge;
  long t1, t2, t3;

  // Для этого пользуемся монотонным таймером, высчитав разницу между границами
  // минуты у него и у таймера реального времени.
  GetTimerValueByClockId(CLOCK_REALTIME, time_current_realtime);
  t1 = time_current_realtime.tv_sec / 60;
  t2 = t1 * 60;
  t3 = t2 + 60;

/*  std::cout << time_current_realtime.tv_sec << "\t"
            << "t1=" << t1 << "\t"
            << "t2=" << t2 << "\t"
            << "t3=" << t3 << "\t"
            << std::endl;*/

  // До границы минуты остается:
  seconds_to_minute_edge = t3 - time_current_realtime.tv_sec;
  if (60 == seconds_to_minute_edge) seconds_to_minute_edge = 0;
  //std::cout << "seconds_to_minute_edge = " << seconds_to_minute_edge << std::endl;
  return seconds_to_minute_edge;
}

// =================================================================================
long get_seconds_to_minute_edge()
{
  long seconds;
  long delta_realtime = get_realtime_seconds_to_minute_edge();
  // NB: пока не используется
  //long delta_monotonic = get_monotonic_seconds_to_minute_edge();

  seconds = delta_realtime;
  return seconds;
}


/* mco_system_get_current_time() must be implmeneted by the
 * application in order to use the High Availability runtime.
 * You may remove this function if you are not using  the
 * HA-enabled runtime
 */
long mco_system_get_current_time(void)
{
    printf("CALL mco_system_get_current_time\n");
    return 0;
}
