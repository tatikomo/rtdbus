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
