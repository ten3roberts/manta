#include "math/vec.h"
#include "math/math.h"

// Returns the normalized vector of a
vec2 vec2_norm(vec2 a)
{
	float mag = sqrtf(a.x * a.x + a.y * a.y);
	return (vec2){a.x / mag, a.y / mag};
}

// Converts a vector to a comma separated string with the components
void vec2_string(vec2 a, char * buf, int precision)
{
	buf += ftos(a.x, buf, precision);
	*buf++ = ',';
	*buf++ = ' ';
	buf += ftos(a.y, buf, precision);
}

// Converts a vector to a comma separated string with the components and magnitude
void vec2_string_long(vec2 a, char * buf, int precision)
{
	buf += ftos(a.x, buf, precision);
	*buf++ = ',';
	*buf++ = ' ';
	buf += ftos(a.y, buf, precision);
	*buf++ = ';';
	*buf++ = ' ';
	buf += ftos(vec2_mag(a), buf, precision);
}