/*
 * g++ -g a.c timer.cpp -I. -I../cmake -o a -lrt
 */
#include <stdio.h>

#include "helper.hpp"

int main()
{
  timer_unit_t ti;
  int res = GetTimerValue(ti);

  printf("%ld %d ti.tv_sec = %ld %ld\n",
    _POSIX_C_SOURCE, res, ti.tv_sec, ti.tv_nsec);
  return 0;
}
