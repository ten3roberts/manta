#pragma once
#include <stdbool.h>
#include <math.h>

#define GIGABYTE 1073741824
#define MEGABYTE 1048576
#define KILOBYTE 1048576

#define GIGA 1000000000
#define MEGA 1000000
#define KILO 1000

inline float clampf(float f, float min, float max)
{
    return (f < min ? min : f > max ? max : f);
}

inline int clampi(int f, int min, int max)
{
	return (f < min ? min : f > max ? max : f);
}

#define DEG_360 (2*M_PI)
#define DEG_180 (M_PI)
#define DEG_90 (M_PI/2)
#define DEG_45 (M_PI/4)

#if PL_LINUX
static void itoa(int a, char* buf, int base)
{
	strcpy(buf, "TODO : imp itoa");
}
#endif