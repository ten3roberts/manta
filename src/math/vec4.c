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

vec4 vec4_hsv(float h, float s, float v)
{
	vec4 out;
	out.w = 1.0f;

	h = fmod(h, M_PI * 2);
	float p, q, t, ff;
	long i;

	if (s <= 0.0f)
	{ //  < is bogus, just shuts up warnings
		out.x = v;
		out.y = v;
		out.z = v;
		return out;
	}
	if (h >= M_PI * 2)
		h = 0.0f;
	h /= (M_PI * 2) / 6.0f;
	i = (long)h;
	ff = h - i;
	p = v * (1.0f - s);
	q = v * (1.0f - (s * ff));
	t = v * (1.0f - (s * (1.0f - ff)));

	switch (i)
	{
	case 0:
		out.x = v;
		out.y = t;
		out.z = p;
		break;
	case 1:
		out.x = q;
		out.y = v;
		out.z = p;
		break;
	case 2:
		out.x = p;
		out.y = v;
		out.z = t;
		break;

	case 3:
		out.x = p;
		out.y = q;
		out.z = v;
		break;
	case 4:
		out.x = t;
		out.y = p;
		out.z = v;
		break;
	case 5:
	default:
		out.x = v;
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