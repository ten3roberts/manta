#include "math/vec.h"
#include "math/math.h"

// Converts a vector to a comma separated string with the components
void vec4_string(vec4 a, char * buf, int precision)
{
	buf += ftos(a.x, buf, precision);
	*buf++ = ',';
	*buf++ = ' ';
	buf += ftos(a.y, buf, precision);
	*buf++ = ',';
	*buf++ = ' ';
	buf += ftos(a.z, buf, precision);
	*buf++ = ',';
	*buf++ = ' ';
	buf += ftos(a.w, buf, precision);
}

// Converts a vector to a comma separated string with the components and magnitude
void vec4_string_long(vec4 a, char * buf, int precision)
{
	buf += ftos(a.x, buf, precision);
	*buf++ = ',';
	*buf++ = ' ';
	buf += ftos(a.y, buf, precision);
	*buf++ = ',';
	*buf++ = ' ';
	buf += ftos(a.z, buf, precision);
	*buf++ = ',';
	*buf++ = ' ';
	buf += ftos(a.w, buf, precision);
	*buf++ = ';';
	*buf++ = ' ';
	buf += ftos(vec4_mag(a), buf, precision);
}
