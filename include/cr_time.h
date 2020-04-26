#ifndef CR_TIME_H
#define CR_TIME_H
#include <time.h>
#include <stdint.h>

// Initializes and starts time
// Sets all values to zero
// Can be called multiple times, e.g; when switching levels
void time_init();

// Updates time and moves to the next frame
void time_update();

// Returns the time point when time was initialized
clock_t time_init_time();

// Returns the time elapsed since init
float time_elapsed();

// Returns the time between the start of this fram and the previous
float time_delta();

// Returns the current framrate
float time_framerate();

// Returns the amount of frames elapsed
size_t time_framecount();

#if PL_LINUX
#include <unistd.h>
// Sleeps for (float) seconds
#define SLEEP(s) usleep(s * 1000000)
#elif PL_WINDOWS
#include <Windows.h>
// Sleeps for (float) seconds
#define SLEEP(s) Sleep(s * 1000);
#endif
#endif