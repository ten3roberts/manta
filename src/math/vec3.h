#pragma once

#include "vec4.h"
#include "math_extra.h"
#include <math.h>

#include <stdlib.h>

typedef struct
{
	float x, y, z;
} vec3;

const static vec3 vec3_one = {1, 1, 1};
const static vec3 vec3_zero = {0, 0, 0};

const static vec3 vec3_forward = {0, 0, 1};
const static vec3 vec3_right = {1, 0, 0};
const static vec3 vec3_up = {0, 1, 0};

const static vec3 vec3_red = {1, 0, 0};
const static vec3 vec3_green = {0, 1, 0};
const static vec3 vec3_blue = {0, 0, 1};

vec3 vec3_add(vec3 a, vec3 b)
{
	return (vec3){a.x + b.x, a.y + b.y, a.z + b.z};
}

vec3 vec3_subtract(vec3 a, vec3 b)
{
	return (vec3){a.x - b.x, a.y - b.y, a.z - b.z};
}

// Calculates the pairwise vector product
vec3 vec3_prod(vec3 a, vec3 b)
{
	return (vec3){a.x * b.x, a.y * b.y, a.z * b.z};
}

// Returns the vector scaled with b
vec3 vec3_scale(vec3 a, float b)
{
	return (vec3){a.x * b, a.y * b, a.z * b};
}

// Returns the normalized vector a
vec3 vec3_norm(vec3 a)
{
	float mag = sqrt(a.x * a.x + a.y * a.y + a.z * a.z);
	return (vec3){a.x / mag, a.y / mag, a.z / mag};
}

float vec3_dot(vec3 a, vec3 b)
{
	return a.x * b.x + a.y * b.y + a.z * b.z;
}

vec3 vec3_cross(vec3 a, vec3 b)
{
	return (vec3){a.y * b.z - a.z * b.y, a.z * b.x - a.x * b.z, a.x * b.y - a.y * b.x};
}

vec3 vec3_reflect(vec3 ray, vec3 normal)
{
	vec3 n = vec3_norm(normal);
	return vec3_subtract(ray, vec3_scale(vec3_scale(n,2), vec3_dot(ray, n)));
}

vec3 vec3_lerp(vec3 a, vec3 b, float t)
{
	t = clampf(t, 0, 1);
	return vec3_add(vec3_scale(a, 1-t), vec3_scale(b,t));
}

// Calculates the magnitude of a vector
float vec3_mag(vec3 a)
{
	return sqrt(a.x * a.x + a.y * a.y + a.z * a.z);
}

// Calculates the squared magnitude of a vector
// Is faster than vec3_mag and can be used for comparisons
float vec3_sqrmag(vec3 a)
{
	return a.x * a.x + a.y * a.y + a.z * a.z;
}

// Returns a random point inside a cube
vec3 vec3_random_cube(float width)
{
	vec3 result;
	result.x = ((float)rand() / RAND_MAX * 2 - 1) * width;
	result.y = ((float)rand() / RAND_MAX * 2 - 1) * width;
	result.z = ((float)rand() / RAND_MAX * 2 - 1) * width;
	return result;
}

// Returns a random point in a sphere
vec3 vec3_random_sphere_even(float minr, float maxr)
{
	vec3 result = {2.0f * (float)rand() / RAND_MAX - 1.0f,

				   2.0f * (float)rand() / RAND_MAX - 1.0f,

				   2.0f * (float)rand() / RAND_MAX - 1.0f};

	result = vec3_norm(result);

	float randomLength = sqrt(((float)rand() / RAND_MAX) / M_PI); // Accounting sparser distrobution
	result = vec3_scale(result, randomLength * (maxr - minr) + minr);

	return result;
}

// Returns a vector with a random direction and magnitude
vec3 vec3_random_sphere(float minr, float maxr)
{
	vec3 result = {2.0f * (float)rand() / RAND_MAX - 1.0f,

				   2.0f * (float)rand() / RAND_MAX - 1.0f,

				   2.0f * (float)rand() / RAND_MAX - 1.0f};

	result = vec3_norm(result);

	float randomLength = (float)rand() / RAND_MAX;
	result = vec3_scale(result, randomLength * (maxr - minr) + minr);

	return result;
}

vec3 to_vec3(vec4 a)
{
	return (vec3){a.x,a.y,a.z};
}

vec4 to_vec4(vec3 a, float w)
{
	return (vec4){a.x,a.y,a.z, w};
}