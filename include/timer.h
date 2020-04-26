#ifndef TIMER_H
#define TIMER_H

#include <time.h>
#include <stdint.h>

typedef enum
{
	// Measures the CPU execution time
	CT_EXECUTION_TICKS,
	// Measures the monotonic wall clock
	CT_WALL_TICKS
} ClockType;

// Defines a timer which can be used to time functions and create intervals
// There are two modes
// CT_EXECUTION_TICKS times the time taken executing a function.
// CT_WALL_TICKS
// timer_start starts a timer
// timer_duration() reads the current elapsed timer from the timer
// timer_stop stops a timer and returns its elapsed time
// A stopped timer's elapsed time can be reread with timer_duration
// timer_reset will reset a timer's elapsed time, and it if it stopped, start it again
typedef struct
{
	ClockType type;
	uint64_t start_tick;
	uint64_t end_tick;
	uint64_t freq;
	short running;
} Timer;

// Starts and returns a timer
Timer timer_start(ClockType type);

// Stops a timer and returns the time passed
float timer_stop(Timer* timer);

// Resets the timer and starts it again if stopped
// Can be used on a running clock
void timer_reset(Timer* timer);

// Reads the duration of a timer
// If timer is stopped, it will read the time when the timer was stopped
// If timer is running, it will read the current time elapsed
float timer_duration(const Timer* timer);

// Reads the duration of a stopped timer in CPU ticks
uint64_t timer_ticks(const Timer* timer);
#endif