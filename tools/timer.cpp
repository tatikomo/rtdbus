#include <time.h>
#include <errno.h>
#include <stdio.h>
#include "config.h"
#include "timer.hpp"

int GetTimerValue(timer_unit_t& timer)
{
  struct timespec res;
  int status;

  if (-1 != (status = clock_gettime(CLOCK_TYPE, &res)))
  {
    timer.tv_sec = res.tv_sec;
    timer.tv_nsec = res.tv_nsec;
  }
  else perror("GetTimerValue");

  return status;
}

