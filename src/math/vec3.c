#include "math/vec.h"
#include "math/math.h"
#include <stdlib.h>

// Returns the largest component
// Uses the least amount of comparations
float vec3_largest(vec3 a)
{
	if (a.x > a.y)
	{
		if (a.x > a.z)
			return a.x;
		else
			return a.z;
	}
	else if (a.y > a.x)
	{
		if (a.y > a.z)
			return a.y;
		return a.z;
	}
	else if (a.z > a.x)
	{
		if (a.z > a.y)
			return a.z;
		return a.y;
	}
	// All are equal
	return a.x;
}

// Returns the smallest component
// Uses the least amount of comparations
float vec3_smallest(vec3 a)
{
	if (a.x < a.y)
	{
		if (a.x < a.z)
			return a.x;
		else
			return a.z;
	}
	else if (a.y < a.x)
	{
		if (a.y < a.z)
			return a.y;
		return a.z;
	}
	else if (a.z < a.x)
	{
		if (a.z < a.y)
			return a.z;
		return a.y;
	}
	// All are equal
	return a.x;
}

// Returns the normalized vector a
vec3 vec3_norm(vec3 a)
{
	float mag = sqrtf(a.x * a.x + a.y * a.y + a.z * a.z);
	return (vec3){a.x / mag, a.y / mag, a.z / mag};
}

// Reflects a vector about a normal
vec3 vec3_reflect(vec3 ray, vec3 n)
{
	n = vec3_norm(n);
	return vec3_sub(ray, vec3_scale(vec3_scale(n, 2), vec3_dot(ray, n)));
}

// Projects vector a onto vector b
vec3 vec3_proj(vec3 a, vec3 b)
{
	b		  = vec3_norm(b);
	float dot = vec3_dot(a, b);
	return vec3_scale(b, dot);
}

// Projects vector a onto a plane specified by the normal n
vec3 vec3_proj_plane(vec3 a, vec3 n)
{
	n		  = vec3_norm(n);
	float dot = vec3_dot(a, n);
	return vec3_sub(a, vec3_scale(n, dot));
}

// Linearly interpolates between two vectors
vec3 vec3_lerp(vec3 a, vec3 b, float t)
{
	t = clampf(t, 0, 1);
	return vec3_add(vec3_scale(a, 1 - t), vec3_scale(b, t));
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
	i  = (long)hh;
	ff = hh - i;
	p  = hsv.z * (1.0f - hsv.y);
	q  = hsv.z * (1.0f - (hsv.y * ff));
	t  = hsv.z * (1.0f - (hsv.y * (1.0f - ff)));

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
	result			   = vec3_scale(result, randomLength * (maxr - minr) + minr);

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
	result			   = vec3_scale(result, randomLength * (maxr - minr) + minr);

	return result;
}

// Truncates a vec4 to a vec3, discarding the w component
vec3 to_vec3(vec4 a)
{
	return (vec3){a.x, a.y, a.z};
}

// Creates a vec4 from a vec3 and a w component
vec4 to_vec4(vec3 a, float w)
{
	return (vec4){a.x, a.y, a.z, w};
}

// Converts a vector to a comma separated string with the components
void vec3_string(vec3 a, char* buf, int precision)
{
	buf += ftos(a.x, buf, precision);
	*buf++ = ',';
	*buf++ = ' ';
	buf += ftos(a.y, buf, precision);
	*buf++ = ',';
	*buf++ = ' ';
	buf += ftos(a.z, buf, precision);
}

// Converts a vector to a comma separated string with the components and magnitude
void vec3_string_long(vec3 a, char* buf, int precision)
{
	buf += ftos(a.x, buf, precision);
	*buf++ = ',';
	*buf++ = ' ';
	buf += ftos(a.y, buf, precision);
	*buf++ = ',';
	*buf++ = ' ';
	buf += ftos(a.z, buf, precision);
	*buf++ = ';';
	*buf++ = ' ';
	buf += ftos(vec3_mag(a), buf, 3);
}
