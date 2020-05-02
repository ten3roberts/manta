#include "math/quaternion.h"
#include <math.h>


// Returns the normalized quaternion a
quaternion quat_norm(quaternion a)
{
	float magnitude = sqrtf(a.x * a.x + a.y * a.y + a.z * a.z + a.w * a.w);

	return (quaternion) { a.x / magnitude, a.y / magnitude, a.z / magnitude, a.w / magnitude };
}

// Returns the quaternions magnitude
// For rotations, a quaternion's magnitude shall be 1
float quat_mag(quaternion a)
{
	return sqrtf(a.x * a.x + a.y * a.y + a.z * a.z + a.w * a.w);
}

// Inverses the quaternion's rotation as if the angular rotation was inverses
quaternion quat_inverse(quaternion a)
{
	return (quaternion) { a.x, a.y, a.z, -a.w };
}

// Calculates the dot product between two quaternions
float quat_dot(quaternion a, quaternion b)
{
	return a.x * b.x + a.y * b.y + a.z * b.z + a.w * b.w;
}

// Adds two quaternions together component wise
quaternion quat_add(quaternion a, quaternion b)
{
	return (quaternion) { a.x + b.x, a.y + b.y, a.z + b.z, a.w + b.w };
}

// Subtracts two quaternions component wise
quaternion quat_sub(quaternion a, quaternion b)
{
	return (quaternion) { a.x + b.x, a.y + b.y, a.z + b.z, a.w + b.w };
}

// Returns a quaternion constructed from an axis angle rotation
quaternion quat_axis_angle(vec3 axis, float angle)
{
	float sinAngle = sinf(angle / 2);
	quaternion result = { axis.x * sinAngle, axis.y * sinAngle, axis.z * sinAngle, cosf(angle / 2) };
	float magnitude = sqrtf(result.x * result.x + result.y * result.y + result.z * result.z + result.w * result.w);

	return (quaternion) { result.x / magnitude, result.y / magnitude, result.z / magnitude, result.w / magnitude };
}

// Returns a quaternion constructed from euler angles (YXZ)
quaternion quat_euler(vec3 euler)
{
	float Y = sinf(euler.y / 2);
	float X = sinf(euler.x / 2);
	float Z = sinf(euler.z / 2);

	float ey = cosf(euler.y / 2);
	float ex = cosf(euler.x / 2);
	float ez = cosf(euler.z / 2);
	return (quaternion) {
		+ey * X * ez + Y * ex * Z, -ey * X * Z + Y * ex * ez, -Y * X * ez + ey * ex * Z,
			+Y * X * Z + ey * ex * ez
	};
}

// Returns an euler angle representation of the given quaternion
vec3 quat_to_euler(quaternion a)
{
	vec3 result;
	//  roll (x-axis rotation)
	float sinr_cosp = +2.0f * (a.w * a.x + a.y * a.z);
	float cosr_cosp = +1.0f - 2.0f * (a.x * a.x * a.y * a.y);
	result.z = atan2f(sinr_cosp, cosr_cosp);

	//  pitch (y-axis rotation)
	float sinp = +2.0f * (a.w * a.y - a.z * a.x);
	if (fabs(sinp) >= 1)
		result.x = copysignf(M_PI / 2, sinp); //  use 90 degrees if out of range
	else
		result.x = asinf(sinp);

	//  yaw (z-axis rotation)
	float siny_cosp = +2.0f * (a.w * a.z + a.x * a.y);
	float cosy_cosp = +1.0f - 2.0f * (a.y * a.y + a.z * a.z);
	result.y = atan2f(siny_cosp, cosy_cosp);
	return result;
}

// Converts a quaternion to a rotation matrix
mat4 quat_to_mat4(quaternion a)
{
	return (mat4) {
		{ {1 - 2 * a.y * a.y - 2 * a.z * a.z, 2 * a.x * a.y - 2 * a.z * a.w, 2 * a.x * a.z + 2 * a.y * a.w, 0},
		{ 2 * a.x * a.y + 2 * a.z * a.w, 1 - 2 * a.x * a.x - 2 * a.z * a.z, 2 * a.y * a.z - 2 * a.x * a.w, 0 },
		{ 2 * a.x * a.z - 2 * a.y * a.w, 2 * a.y * a.z + 2 * a.x * a.w, 1 - 2 * a.x * a.x - 2 * a.y * a.y, 0 },
		{ 0, 0, 0, 1 }}
	};
}

// Returns a quaternion that can transform a forward vector to rotate to point
quaternion quat_point_to(vec3 a)
{
	vec3 forward_vector = vec3_norm(a);

	float dot = vec3_dot(vec3_forward, forward_vector);

	if (fabs(dot - (-1.0f)) < 0.000001f)
	{
		return (quaternion) { 0, 1, 0, M_PI };
	}
	if (fabs(dot - (1.0f)) < 0.000001f)
	{
		return quat_identity;
	}

	float rot_angle = (float)acos(dot);
	vec3 rot_axis = vec3_cross(vec3_forward, forward_vector);
	rot_axis = vec3_norm(rot_axis);
	return quat_axis_angle(rot_axis, rot_angle);
}

// Multiplies two quaternions
// This is equivalent to the two rotations they represent being carried out after one another
quaternion quat_mul(quaternion a, quaternion b)
{
	return (quaternion) {
		a.x* b.w + a.y * b.z - a.z * b.y + a.w * b.x, -a.x * b.z + a.y * b.w + a.z * b.x + a.w * b.y,
			a.x* b.y - a.y * b.x + a.z * b.w + a.w * b.z, -a.x * b.x - a.y * b.y - a.z * b.z + a.w * b.w
	};
}

// Transforms a vec3 with the quaternion rotation
vec3 quat_vec3_mul(quaternion a, vec3 b)
{

	vec3 t = vec3_scale(vec3_cross((vec3) { a.x, a.y, a.z }, b), 2.0f);
	return vec3_add(vec3_add(b, vec3_scale(t, a.w)), vec3_cross((vec3) { a.x, a.y, a.z }, t));
}

// Scales a quaternion component wise
quaternion quat_scale(quaternion a, float b)
{
	return (quaternion) { a.x* b, a.y* b, a.z* b, a.w* b };
}

// Scales the angle of rotation while preserving the rotation axis
quaternion quat_ang_scale(quaternion a, float b)
{
	float angle = acosf(a.w) * b;
	quaternion out;
	float old_half_sin = sqrtf(1 - a.w * a.w);
	float half_sin = sinf(angle / 2);
	out.x = a.x / old_half_sin * half_sin;
	out.y = a.y / old_half_sin * half_sin;
	out.z = a.z / old_half_sin * half_sin;
	out.w = cosf(angle / 2);
	return out;
}

// Spherically linearly interpolates between two rotations
quaternion quat_slerp(quaternion a, quaternion b, float t)
{
	//  Only unit quaternions are valid rotations.
	//  Normalize to avoid undefined behavior.
	a = quat_norm(a);
	b = quat_norm(b);

	//  get the dot product between the two quaternions
	float dot = quat_dot(a, b);

	//  Makes it take the shortest path
	if (dot < 0)
	{
		b = quat_scale(b, -1);
		dot = -dot;
	}

	const float CLOSEST_DOT = 0.9995f;
	if (dot > CLOSEST_DOT)
	{
		//  If the inputs are too close, linearly interpolate
		//  and Normalize the result.

		quaternion result = quat_add(a, quat_scale(quat_sub(b, a), t));
		result = quat_norm(result);
		return result;
	}

	//  acos is safe because the cos between a, b is less than closetsDot
	float angle = acosf(dot);			   //  theta_0 = angle between input vectors
	float partAngle = angle * t;		   //  theta = angle between v0 and result
	float sinPart = sinf(partAngle);	   //  compute this value only once sinTheta
	float sinAngle = sqrtf(1 - dot * dot); //  compute this value only once sintheta0

	float s0 = cosf(partAngle) - dot * sinPart / sinAngle; //  == sin(theta_0 - theta) / sin(theta_0)
	float s1 = sinPart / sinAngle;

	return quat_add(quat_scale(a, s0), quat_scale(b, s1));
}

// Linearly interpolates two rotations
// Is faster but does not produce the same result as SLERP
quaternion quat_lerp(quaternion a, quaternion b, float t)
{
	//  If the inputs are too close, linearly interpolate
	//  and Normalize the result.
	t = clampf(t, 0, 1);
	quaternion result = quat_add(a, quat_scale(quat_sub(b, a), t));
	result = quat_norm(result);
	return result;
}