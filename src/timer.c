#include "timer.h"
#include "log.h"
#if PL_WINDOWS
#include <windows.h>
#define GET_TICKS (GetTickCount64() * (CLOCKS_PER_SEC / 1000))
#elif PL_LINUX
#endif
Timer timer_start(ClockType type)
{
	Timer timer;
	timer.type = type;
	if (type == CT_EXECUTION_TICKS)
	{
		timer.start_tick = clock();
	}
	else
	{
		timer.start_tick = GET_TICKS;
	}
	timer.end_tick = 0;

	timer.running = 1;

	return timer;
}

float timer_stop(Timer* timer)
{
	if (timer->type == CT_EXECUTION_TICKS)
	{
		timer->end_tick = clock();
		timer->running = 0;
	}
	else
	{
		timer->end_tick = GET_TICKS;
	}
	return (timer->end_tick - timer->start_tick) / (float)CLOCKS_PER_SEC;
}

void timer_reset(Timer* timer)
{
	if (timer->type == CT_EXECUTION_TICKS)
	{
		timer->start_tick = clock();
		timer->end_tick = 0;
		timer->running = 1;
	}
	else
	{
		timer->start_tick = GET_TICKS;
		timer->end_tick = 0;
		timer->running = 1;
	}
}

float timer_duration(const Timer* timer)
{
	if (timer->running == 0)
		return (timer->end_tick - timer->start_tick) / (float)CLOCKS_PER_SEC;
	if (timer->type == CT_EXECUTION_TICKS)
		return (clock() - timer->start_tick) / (float)CLOCKS_PER_SEC;
	// Wall time
	return ((GET_TICKS)-timer->start_tick) / (float)CLOCKS_PER_SEC;
}

clock_t timer_ticks(const Timer* timer)
{
	return timer->end_tick - timer->start_tick;
}