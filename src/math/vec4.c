#include "math/vec.h"
#include "math/math.h"

// Converts a vector to a comma separated string with the components
void vec4_string(vec4 a, char* buf, int precision)
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
void vec4_string_long(vec4 a, char* buf, int precision)
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

vec4 vec4_hsv(vec3 hsv)
{
	vec4 out;
	out.z = 1.0f;

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

vec4 vec4_norm(vec4 a)
{
	float mag = sqrtf(a.x * a.x + a.y * a.y + a.z * a.z + a.w * a.w);
	return (vec4){a.x / mag, a.y / mag, a.z / mag, a.w / mag};
}