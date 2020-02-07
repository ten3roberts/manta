#pragma once
#define _USE_MATH_DEFINES
#include <math.h>
#include <stdbool.h>

#define GIGABYTE 1073741824
#define MEGABYTE 1048576
#define KILOBYTE 1048576

#define GIGA 1000000000
#define MEGA 1000000
#define KILO 1000

static inline float clampf(float f, float min, float max)
{
	return (f < min ? min : f > max ? max : f);
}

static inline int clampi(int f, int min, int max)
{
	return (f < min ? min : f > max ? max : f);
}

static float logn(float base, float x)
{
	return log(x) / log(base);
}

#define DEG_360 (2 * M_PI)
#define DEG_180 (M_PI)
#define DEG_90 (M_PI / 2)
#define DEG_45 (M_PI / 4)

#define min(a, b) (a < b ? a : b)
#define max(a, b) (a > b ? a : b)

// Converts a signed integer to a string
// Returns how many characters were written
int itos(signed long long num, char * buf, int base, int upper);

// Converts an unsigned integer to a string
// Returns how many characters were written
int utos(unsigned long long num, char * buf, int base, int upper);

// Converts a double/float to a string
// Precision indicates the max digits to include after the comma
// Prints up to precision digits after the comma, can write less. Can be used to print integers, where the comma is not written
// Returns how many characters were written
int ftos(double num, char * buf, int precision);

// Converts a double/float to a string
// Writes a fixed length number
int ftos_fixed(double num, char * buf, int length);

// Converts a double/float to a string
// Precision indicates the max digits to include after the comma
// Prints up to precision digits after the comma, can write less. Can be used to print integers, where the comma is not written
// Writes a minimum of pad characters
// pad_length is recommended to be 2 less than precisio nto fit 0.
// Returns how many characters were written
int ftos_pad(double num, char * buf, int precision, int pad_length, char pad_char);
