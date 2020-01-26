#include "timer.h"

Timer timer_start()
{
	Timer timer;
	timer.start = clock();
	timer.end = 0;
	timer.running = 1;
	return timer;
}

float timer_stop(Timer* timer)
{
	timer->end = clock();
	timer->running = 0;
	return (timer->end - timer->start) / (float)CLOCKS_PER_SEC;
}

float timer_duration(Timer* timer)
{
	return (timer->end - timer->start) / (float)CLOCKS_PER_SEC;
}

clock_t timer_ticks(Timer* timer)
{
	return timer->end - timer->start;
}