#include "timer.h"
#if PL_WINDOWS
#include <windows.h>
#define MILLION 1000000
#define GET_FREQ(f) (QueryPerformanceFrequency(&f))
#define GET_TICKS(t) (QueryPerformanceCounter(&t))
#elif PL_LINUX
void _get_ticks(uint64_t* t)
{
	struct timespec ts;
	clock_gettime(CLOCK_MONOTONIC_RAW, &ts);
	*t = ts.tv_sec * MILLION + ts.tv_nsec / 1000;
}
#define GET_FREQ(f) MILLION
#define GET_TICKS(t) _get_ticks(&t)
#endif
Timer timer_start(ClockType type)
{
	Timer timer;
	timer.type = type;
	if (type == CT_EXECUTION_TICKS)
	{
		timer.freq = CLOCKS_PER_SEC;
		timer.start_tick = clock();
	}
	else
	{
		GET_FREQ(timer.freq);
		GET_TICKS(timer.start_tick);
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
		GET_TICKS(timer->end_tick);
	}
	return (timer->end_tick - timer->start_tick) / (float)timer->freq;
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
		GET_TICKS(timer->start_tick);
		timer->end_tick = 0;
		timer->running = 1;
	}
}

float timer_duration(const Timer* timer)
{
	if (timer->running == 0)
		return (timer->end_tick - timer->start_tick) / (float)timer->freq;
	if (timer->type == CT_EXECUTION_TICKS)
		return (clock() - timer->start_tick) / (float)timer->freq;
	// Wall time
	uint64_t now_tick;
	GET_TICKS(now_tick);
	return (now_tick-timer->start_tick) / (float)timer->freq;
}

uint64_t timer_ticks(const Timer* timer)
{
	return timer->end_tick - timer->start_tick;
}