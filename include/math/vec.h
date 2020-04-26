#ifndef VEC_H
#define VEC_H

#include "math.h"
#include <math.h>

typedef struct
{
	float x, y;
} vec2;

typedef struct
{
	int x, y;
} ivec2;

const static vec2 vec2_one = { 1, 1 };
const static vec2 vec2_zero = { 0, 0 };

typedef struct
{
	float x, y, z;
} vec3;
typedef struct
{
	int x, y, z;
} ivec3;

const static vec3 vec3_one = { 1, 1, 1 };
const static vec3 vec3_zero = { 0, 0, 0 };

const static vec3 vec3_forward = { 0, 0, 1 };
const static vec3 vec3_right = { 1, 0, 0 };
const static vec3 vec3_up = { 0, 1, 0 };

const static vec3 vec3_red = { 1, 0, 0 };
const static vec3 vec3_green = { 0, 1, 0 };
const static vec3 vec3_blue = { 0, 0, 1 };

typedef struct
{
	float x, y, z, w;
} vec4;
typedef struct
{
	int x, y, z, w;
} ivec4;

const static vec4 vec4_one = { 1, 1, 1 };
const static vec4 vec4_zero = { 0, 0, 0 };

const static vec4 vec4_red = { 1, 0, 0, 1 };
const static vec4 vec4_green = { 0, 1, 0, 1 };
const static vec4 vec4_blue = { 0, 0, 1, 1 };

/*vec2*/

// Adds two vectors together component wise
static inline vec2 vec2_add(vec2 a, vec2 b)
{
	return (vec2) { a.x + b.x, a.y + b.y };
}

// Subtracts two vectors component wise
static inline vec2 vec2_sub(vec2 a, vec2 b)
{
	return (vec2) { a.x - b.x, a.y - b.y };
}

// Calculates the pairwise vector product
static inline vec2 vec2_prod(vec2 a, vec2 b)
{
	return (vec2) { a.x* b.x, a.y* b.y };
}

// Returns the vector scaled with b
static inline vec2 vec2_scale(vec2 a, float b)
{
	return (vec2) { a.x* b, a.y* b };
}

// Calculates the magnitude of a vector
static inline float vec2_mag(vec2 a)
{
	return sqrtf(a.x * a.x + a.y * a.y);
}

// Calculates the squared magnitude of a vector
// Is faster than vec2_mag and can be used for comparisons
static inline float vec2_sqrmag(vec2 a)
{
	return a.x * a.x + a.y * a.y;
}

// Calculates the dot product between two vectors
static inline float vec2_dot(vec2 a, vec2 b)
{
	return a.x * b.x + a.y * b.y;
}

// Returns the normalized vector of a
vec2 vec2_norm(vec2 a);

// Converts a vector to a comma separated string with the components
void vec2_string(vec2 a, char* buf, int precision);

// Converts a vector to a comma separated string with the components and magnitude
void vec2_string_long(vec2 a, char* buf, int precision);

/*vec3*/

// Adds two vectors together component wise
static inline vec3 vec3_add(vec3 a, vec3 b)
{
	return (vec3) { a.x + b.x, a.y + b.y, a.z + b.z };
}

// Subtracts two vectors component wise
static inline vec3 vec3_sub(vec3 a, vec3 b)
{
	return (vec3) { a.x - b.x, a.y - b.y, a.z - b.z };
}

// Calculates the pairwise vector product
static inline vec3 vec3_prod(vec3 a, vec3 b)
{
	return (vec3) { a.x* b.x, a.y* b.y, a.z* b.z };
}

// Returns the vector scaled with b
static inline vec3 vec3_scale(vec3 a, float b)
{
	return (vec3) { a.x* b, a.y* b, a.z* b };
}

// Calculates the dot product between two vectors
static inline float vec3_dot(vec3 a, vec3 b)
{
	return a.x * b.x + a.y * b.y + a.z * b.z;
}

// Calculates the cross product of two vectors
// The cross product produces a vector perpendicular to both vectors
static inline vec3 vec3_cross(vec3 a, vec3 b)
{
	return (vec3) { a.y* b.z - a.z * b.y, a.z* b.x - a.x * b.z, a.x* b.y - a.y * b.x };
}
// Calculates the magnitude of a vector
static inline float vec3_mag(vec3 a)
{
	return sqrtf(a.x * a.x + a.y * a.y + a.z * a.z);
}

// Calculates the squared magnitude of a vector
// Is faster than vec3_mag and can be used for comparisons
static inline float vec3_sqrmag(vec3 a)
{
	return a.x * a.x + a.y * a.y + a.z * a.z;
}
// Returns the normalized vector a
vec3 vec3_norm(vec3 a);

// Reflects a vector about a normal
vec3 vec3_reflect(vec3 ray, vec3 n);

// Projects vector a onto vector b
vec3 vec3_proj(vec3 a, vec3 b);

// Projects vector a onto a plane specified by the normal n
vec3 vec3_proj_plane(vec3 a, vec3 n);

// Linearly interpolates between two vectors
vec3 vec3_lerp(vec3 a, vec3 b, float t);

// Converts an HSV value to RGB
vec3 vec3_hsv(vec3 hsv);

// Returns a random point inside a cube
vec3 vec3_random_cube(float width);

// Returns a random point in a sphere
vec3 vec3_random_sphere_even(float minr, float maxr);

// Returns a vector with a random direction and magnitude
vec3 vec3_random_sphere(float minr, float maxr);

// Truncates a vec4 to a vec3, discarding the w component
vec3 to_vec3(vec4 a);

// Creates a vec4 from a vec3 and a w component
vec4 to_vec4(vec3 a, float w);

// Converts a vector to a comma separated string with the components
void vec3_string(vec3 a, char* buf, int precision);

// Converts a vector to a comma separated string with the components and magnitude
void vec3_string_long(vec3 a, char* buf, int precision);

/*vec4*/

// Adds two vectors together component wise
static inline vec4 vec4_add(vec4 a, vec4 b)
{
	return (vec4) { a.x + b.x, a.y + b.y, a.z + b.z, a.w + b.w };
}

// Subtracts two vectors component wise
static inline vec4 vec4_sub(vec4 a, vec4 b)
{
	return (vec4) { a.x - b.x, a.y - b.y, a.z - b.z, a.w - b.w };
}

// Calculates the pairwise vector product
static inline vec4 vec4_prod(vec4 a, vec4 b)
{
	return (vec4) { a.x* b.x, a.y* b.y, a.z* b.z, a.w* b.w };
}

// Returns the vector scaled with b
static inline vec4 vec4_scale(vec4 a, float b)
{
	return (vec4) { a.x* b, a.y* b, a.z* b, a.w* b };
}

// Calculates the dot product between two vectors
static inline float vec4_dot(vec4 a, vec4 b)
{
	return a.x * b.x + a.y * b.y + a.z * b.z + a.w * b.w;
}

// Calculates the magnitude of a vector
static inline float vec4_mag(vec4 a)
{
	return sqrtf(a.x * a.x + a.y * a.y + a.z * a.z + a.w * a.w);
}

// Calculates the squared magnitude of a vector
// Is faster than vec4_mag and can be used for comparisons
static inline float vec4_sqrmag(vec4 a)
{
	return a.x * a.x + a.y * a.y + a.z * a.z + a.w * a.w;
}

// Returns the normalized vector of a
vec4 vec4_norm(vec4 a);

// Converts a vector to a comma separated string with the components
void vec4_string(vec4 a, char* buf, int precision);

// Converts a vector to a comma separated string with the components and magnitude
void vec4_string_long(vec4 a, char* buf, int precision);
#endif