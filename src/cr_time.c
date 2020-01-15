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
} _Time;

_Time _time = { 0,0,0,0, {0,0}, {0,0}, {0,0} };

void time_init()
{
	clock_gettime(CLOCK_MONOTONIC_RAW, &_time.init_time);
	_time.elapsedtime = 0.0f;
	_time.deltatime = 0.0f;
	_time.framerate = 0.0f;
	_time.framecount = 0;
	_time.prev = _time.init_time;
	_time.now = _time.init_time;
}

void time_update()
{
	// Sets previous time to now
	_time.prev = _time.now;

	// Update now
	clock_gettime(CLOCK_MONOTONIC_RAW, &_time.now);
	_time.deltatime = (_time.now.tv_sec - _time.prev.tv_sec) + (_time.now.tv_nsec - _time.prev.tv_nsec) /
		1000000000.0;

	_time.elapsedtime = (_time.now.tv_sec - _time.init_time.tv_sec) + (_time.now.tv_nsec - _time.init_time.tv_nsec) /
		1000000000.0;

	_time.framerate = 1 / _time.deltatime;
	_time.framecount++;
}

clock_t time_init_time()
{
	return _time.init_time.tv_sec + _time.init_time.tv_nsec / 1000000000.0;
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
} _Time;

_Time _time = { 0,0 ,0,0,0,0,0 };

void time_init()
{

	_time.init_tick = GetTickCount();

	_time.elapsedtime = 0.0f;
	_time.deltatime = 0.0f;
	_time.framerate = 0.0f;
	_time.framecount = 0;
	_time.prev_tick = _time.init_tick;
	_time.now_tick = _time.init_tick;
}

void time_update()
{
	// Sets previous time to now
	_time.prev_tick = _time.now_tick;

	// Update now
	_time.now_tick = GetTickCount();
	_time.deltatime = (_time.now_tick - _time.prev_tick) / 1000.0f;

	_time.elapsedtime = (_time.now_tick - _time.init_tick) / 1000.0f;

	_time.framerate = 1 / _time.deltatime;
	_time.framecount++;
}

clock_t time_init_time()
{
	return _time.init_tick / 1000.0f;
}
#endif

float time_elapsed()
{
	return _time.elapsedtime;
}

float time_delta()
{
	return _time.deltatime;
}

float time_framerate()
{
	return _time.framerate;
}

size_t time_framecount()
{
	return _time.framecount;
}
