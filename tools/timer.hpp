#if !defined GEV_TOOLS_TIMER_HPP
#define GEV_TOOLS_TIMER_HPP
#pragma once

#include <time.h>

typedef struct timespec timer_unit_t;

int GetTimerValue(timer_unit_t&);

#endif

