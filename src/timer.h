#include <time.h>

typedef struct
{
	clock_t start;
	clock_t end;
	short running;
} Timer;

// Starts and returns a timer
Timer timer_start();

// Stops a timer and returns the time passed
float timer_stop(Timer* timer);

// Reads the duration of a stopped timer
float timer_duration(Timer* timer);

// Reads the duration of a stopped timer in CPU ticks
clock_t timer_ticks(Timer* timer);