#pragma once

#include <math.h>
#include <stdlib.h>

typedef struct
{
	float x, y;
} vec2;

const static vec2 vec2_one = {1, 1};
const static vec2 vec2_zero = {0, 0};

vec2 vec2_add(vec2 a, vec2 b)
{
	return (vec2){a.x + b.x, a.y + b.y};
}

vec2 vec2_subtract(vec2 a, vec2 b)
{
	return (vec2){a.x - b.x, a.y - b.y};
}

// Calculates the pairwise vector product
vec2 vec2_prod(vec2 a, vec2 b)
{
	return (vec2){a.x * b.x, a.y * b.y};
}

// Returns the vector scaled with b
vec2 vec2_scale(vec2 a, float b)
{
	return (vec2){a.x * b, a.y * b};
}

// Returns the normalized vector of a
vec2 vec2_norm(vec2 a)
{
	float mag = sqrt(a.x * a.x + a.y * a.y);
	return (vec2){a.x / mag, a.y / mag};
}

float vec2_dot(vec2 a, vec2 b)
{
	return a.x * b.x + a.y * b.y;
}

// Calculates the magnitude of a vector
float vec2_mag(vec2 a)
{
	return sqrt(a.x * a.x + a.y * a.y);
}

// Calculates the squared magnitude of a vector
// Is faster than vec2_mag and can be used for comparisons
float vec2_sqrmag(vec2 a)
{
	return a.x * a.x + a.y * a.y;
}