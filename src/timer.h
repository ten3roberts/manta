#include <time.h>
#include <stdint.h>

typedef enum
{
	// Measures the CPU execution time
	CT_EXECUTION_TICKS, 
	// Measures the monotonic wall clock
	CT_WALL_TICKS
} ClockType;

typedef struct
{
	ClockType type;
	uint64_t start_tick;
	uint64_t end_tick;
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
clock_t timer_ticks(const Timer* timer);