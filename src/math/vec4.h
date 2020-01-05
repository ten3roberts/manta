#include <math.h>
#include <stdlib.h>

typedef struct
{
	float x, y, z, w;
} vec4;

const static vec4 vec4_one = {1, 1, 1};
const static vec4 vec4_zero = {0, 0, 0};

const static vec4 vec4_red = {1, 0, 0, 1};
const static vec4 vec4_green = {0, 1, 0, 1};
const static vec4 vec4_blue = {0, 0, 1, 1};

vec4 vec4_add(vec4 a, vec4 b)
{
	return (vec4){a.x + b.x, a.y + b.y, a.z + b.z, a.w + b.w};
}

vec4 vec4_subtract(vec4 a, vec4 b)
{
	return (vec4){a.x - b.x, a.y - b.y, a.z - b.z, a.w - b.w};
}

// Calculates the pairwise vector product
vec4 vec4_prod(vec4 a, vec4 b)
{
	return (vec4){a.x * b.x, a.y * b.y, a.z * b.z, a.w * b.w};
}

// Returns the vector scaled with b
vec4 vec4_scale(vec4 a, float b)
{
	return (vec4){a.x * b, a.y * b, a.z * b, a.w * b};
}

// Returns the normalized vector of a
vec4 vec4_norm(vec4 a)
{
	float mag = sqrt(a.x * a.x + a.y * a.y + a.z * a.z + a.w * a.w);
	return (vec4){a.x / mag, a.y / mag, a.z / mag};
}

float vec4_dot(vec4 a, vec4 b)
{
	return a.x * b.x + a.y * b.y + a.z * b.z + a.w * b.w;
}

// Calculates the magnitude of a vector
float vec4_mag(vec4 a)
{
	return sqrt(a.x * a.x + a.y * a.y + a.z * a.z + a.w * a.w);
}

// Calculates the squared magnitude of a vector
// Is faster than vec4_mag and can be used for comparisons
float vec4_sqrmag(vec4 a)
{
	return a.x * a.x + a.y * a.y + a.z * a.z + a.w * a.w;
}