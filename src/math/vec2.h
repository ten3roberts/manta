#pragma once

#include <math.h>
#include <stdlib.h>

typedef struct
{
	float x, y;
} vec2;

const static vec2 vec2_one = {1, 1};
const static vec2 vec2_zero = {0, 0};

// Adds two vectors together component wise
vec2 vec2_add(vec2 a, vec2 b)
{
	return (vec2){a.x + b.x, a.y + b.y};
}

// Subtracts two vectors component wise
vec2 vec2_sub(vec2 a, vec2 b)
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
	float mag = sqrtf(a.x * a.x + a.y * a.y);
	return (vec2){a.x / mag, a.y / mag};
}

// Calculates the dot product between two vectors
float vec2_dot(vec2 a, vec2 b)
{
	return a.x * b.x + a.y * b.y;
}

// Calculates the magnitude of a vector
float vec2_mag(vec2 a)
{
	return sqrtf(a.x * a.x + a.y * a.y);
}

// Calculates the squared magnitude of a vector
// Is faster than vec2_mag and can be used for comparisons
float vec2_sqrmag(vec2 a)
{
	return a.x * a.x + a.y * a.y;
}