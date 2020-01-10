#include "cr_time.h"

typedef struct
{
	clock_t init_time;
	float elapsedtime;
	float deltatime;

	float framerate;
	size_t framecount;

	clock_t prev_tick;
	clock_t now_tick;
	clock_t diff_tick;
} _time;

_time cr_time = { 0,0,0,0,0,0,0 };

void time_init()
{
	cr_time.init_time = clock();
	cr_time.elapsedtime = 0.0f;
	cr_time.deltatime = 0.0f;
	cr_time.framerate = 0.0f;
	cr_time.framecount = 0;
	cr_time.prev_tick = clock();
	cr_time.now_tick = clock();
}

void time_update()
{
	cr_time.prev_tick = cr_time.now_tick;
	cr_time.now_tick = clock();
	cr_time.diff_tick = cr_time.now_tick - cr_time.prev_tick;
	cr_time.deltatime = cr_time.diff_tick / (float)CLOCKS_PER_SEC;
	cr_time.elapsedtime = cr_time.now_tick / (float)CLOCKS_PER_SEC;
	cr_time.framerate = 1 / cr_time.deltatime;
	cr_time.framecount++;
}

clock_t time_init_time()
{
	return cr_time.init_time;
}

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
