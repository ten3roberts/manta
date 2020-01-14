#include "cr_time.h"

#ifdef PL_LINUX
#include "sys/time.h"
typedef struct
{
	float elapsedtime;
	float deltatime;

	float framerate;
	size_t framecount;

	struct timespec init_time;
	struct timespec prev;
	struct timespec now;
} _time;

_time cr_time;

void time_init()
{
	clock_gettime(CLOCK_MONOTONIC_RAW, &cr_time.init_time);
	cr_time.elapsedtime = 0.0f;
	cr_time.deltatime = 0.0f;
	cr_time.framerate = 0.0f;
	cr_time.framecount = 0;
	cr_time.prev = cr_time.init_time;
	cr_time.now = cr_time.init_time;
}

void time_update()
{
	// Sets previous time to now
	cr_time.prev = cr_time.now;

	// Update now
	clock_gettime(CLOCK_MONOTONIC_RAW, &cr_time.now);
	cr_time.deltatime = (cr_time.now.tv_sec - cr_time.prev.tv_sec) + (cr_time.now.tv_nsec - cr_time.prev.tv_nsec)/
		1000000000.0;

	cr_time.elapsedtime = (cr_time.now.tv_sec - cr_time.init_time.tv_sec) + (cr_time.now.tv_nsec - cr_time.init_time.tv_nsec)/
		1000000000.0;

	cr_time.framerate = 1 / cr_time.deltatime;
	cr_time.framecount++;
}

clock_t time_init_time()
{
	return cr_time.init_time.tv_sec + cr_time.init_time.tv_nsec/1000000000.0;
}
#elif PL_WINDOWS
#include <window.h>
typedef struct
{
	float elapsedtime;
	float deltatime;

	float framerate;
	size_t framecount;

	DWORD init_tick;
	DWORD prev_tick;
	DWORD now_tick;
} _time;

_time cr_time;

void time_init()
{

	cr_time.init_tick = GetTickCount();

	cr_time.elapsedtime = 0.0f;
	cr_time.deltatime = 0.0f;
	cr_time.framerate = 0.0f;
	cr_time.framecount = 0;
	cr_time.prev_tick = cr_time.init_tick;
	cr_time.now_tick = cr_time.init_tick;
}

void time_update()
{
	// Sets previous time to now
	cr_time.prev_tick = cr_time.now_tick;

	// Update now
	cr_time.now_tick = GetTickCount();
	cr_time.deltatime = (cr_time.now_tick - cr_time.prev_tick)/1000.0f;

	cr_time.elapsedtime = (cr_time.now_tick - cr_time.init_tick);

	cr_time.framerate = 1 / cr_time.deltatime;
	cr_time.framecount++;
}

clock_t time_init_time()
{
	return cr_time.init_tick / 1000.0f;
}
#endif

float time_elapsed()
{
	return cr_time.elapsedtime;
}

float time_delta()
{
	return cr_time.deltatime;
}

float time_framerate()
{
	return cr_time.framerate;
}

size_t time_framecount()
{
	return cr_time.framecount;
}
