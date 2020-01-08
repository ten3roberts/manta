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

// Adds two vectors together component wise
vec3 vec3_add(vec3 a, vec3 b)
{
	return (vec3){a.x + b.x, a.y + b.y, a.z + b.z};
}

// Subtracts two vectors component wise
vec3 vec3_sub(vec3 a, vec3 b)
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
	float mag = sqrtf(a.x * a.x + a.y * a.y + a.z * a.z);
	return (vec3){a.x / mag, a.y / mag, a.z / mag};
}

// Calculates the dot product between two vectors
float vec3_dot(vec3 a, vec3 b)
{
	return a.x * b.x + a.y * b.y + a.z * b.z;
}

// Calculates the cross product of two vectors
// The cross product produces a vector perpendicular to both vectors
vec3 vec3_cross(vec3 a, vec3 b)
{
	return (vec3){a.y * b.z - a.z * b.y, a.z * b.x - a.x * b.z, a.x * b.y - a.y * b.x};
}

// Reflects a vector about a normal
vec3 vec3_reflect(vec3 ray, vec3 n)
{
	n = vec3_norm(n);
	return vec3_sub(ray, vec3_scale(vec3_scale(n,2), vec3_dot(ray, n)));
}

// Projects vector a onto vector b
vec3 vec3_proj(vec3 a, vec3 b)
{
	b = vec3_norm(b);
	float dot = vec3_dot(a, b);
	return vec3_scale(b, dot);
}

// Projects vector a onto a plane specified by the normal n
vec3 vec3_proj_plane(vec3 a, vec3 n)
{
	n = vec3_norm(n);
	float dot = vec3_dot(a, n);
	return vec3_sub(a, vec3_scale(n, dot));
	
}

// Linearly interpolates between two vectors
vec3 vec3_lerp(vec3 a, vec3 b, float t)
{
	t = clampf(t, 0, 1);
	return vec3_add(vec3_scale(a, 1-t), vec3_scale(b,t));
}

// Calculates the magnitude of a vector
float vec3_mag(vec3 a)
{
	return sqrtf(a.x * a.x + a.y * a.y + a.z * a.z);
}

// Calculates the squared magnitude of a vector
// Is faster than vec3_mag and can be used for comparisons
float vec3_sqrmag(vec3 a)
{
	return a.x * a.x + a.y * a.y + a.z * a.z;
}

// Converts an HSV value to RGB
vec3 vec3_hsv(vec3 hsv)
{
	vec3 out;

	hsv.x = fmod(hsv.x, 1.0f);
	float hh, p, q, t, ff;
	long i;

	if (hsv.y <= 0.0f)
	{ //  < is bogus, just shuts up warnings
		out.x = hsv.z;
		out.y = hsv.z;
		out.z = hsv.z;
		return out;
	}
	hh = hsv.x;
	if (hh >= 1)
		hh = 0.0f;
	hh /= 1 / 6.0f;
	i = (long)hh;
	ff = hh - i;
	p = hsv.z * (1.0f - hsv.y);
	q = hsv.z * (1.0f - (hsv.y * ff));
	t = hsv.z * (1.0f - (hsv.y * (1.0f - ff)));

	switch (i)
	{
	case 0:
		out.x = hsv.z;
		out.y = t;
		out.z = p;
		break;
	case 1:
		out.x = q;
		out.y = hsv.z;
		out.z = p;
		break;
	case 2:
		out.x = p;
		out.y = hsv.z;
		out.z = t;
		break;

	case 3:
		out.x = p;
		out.y = q;
		out.z = hsv.z;
		break;
	case 4:
		out.x = t;
		out.y = p;
		out.z = hsv.z;
		break;
	case 5:
	default:
		out.x = hsv.z;
		out.y = p;
		out.z = q;
		break;
	}
	return out;
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

	float randomLength = sqrtf(((float)rand() / RAND_MAX) / M_PI); // Accounting sparser distrobution
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

// Truncates a vec4 to a vec3, discarding the w component
vec3 to_vec3(vec4 a)
{
	return (vec3){a.x,a.y,a.z};
}

// Creates a vec4 from a vec3 and a w component
vec4 to_vec4(vec3 a, float w)
{
	return (vec4){a.x,a.y,a.z, w};
}