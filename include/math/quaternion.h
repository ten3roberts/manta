#ifndef QUATERNION_H
#define QUATERNION_H

#include "mat4.h"
#include "vec.h"

// Specifies the quaternion structure
typedef struct
{
	float x, y, z, w;
} quaternion;

const static quaternion quat_identity = {0, 0, 0, 1};

// Returns the normalized quaternion a
quaternion quat_norm(quaternion a);

// Returns the quaternions magnitude
// For rotations, a quaternion's magnitude shall be 1
float quat_mag(quaternion a);

// Inverses the quaternion's rotation as if the angular rotation was inverses
quaternion quat_inverse(quaternion a);

// Calculates the dot product between two quaternions
float quat_dot(quaternion a, quaternion b);
// Adds two quaternions together component wise
quaternion quat_add(quaternion a, quaternion b);

// Subtracts two quaternions component wise
quaternion quat_sub(quaternion a, quaternion b);

// Returns a quaternion constructed from an axis angle rotation
quaternion quat_axis_angle(vec3 axis, float angle);

// Returns a quaternion constructed from euler angles (YXZ)
quaternion quat_euler(vec3 euler);

// Returns an euler angle representation of the given quaternion
vec3 quat_to_euler(quaternion a);

// Converts a quaternion to a rotation matrix
mat4 quat_to_mat4(quaternion a);

// Returns a quaternion that can transform a forward vector to rotate to point
quaternion quat_point_to(vec3 a);
// Multiplies two quaternions
// This is equivalent to the two rotations they represent being carried out after one another
quaternion quat_mul(quaternion a, quaternion b);

// Transforms a vec3 with the quaternion rotation
vec3 quat_vec3_mul(quaternion a, vec3 b);

// Scales a quaternion component wise
quaternion quat_scale(quaternion a, float b);

// Scales the angle of rotation while preserving the rotation axis
quaternion quat_ang_scale(quaternion a, float b);

// Spherically linearly interpolates between two rotations
quaternion quat_slerp(quaternion a, quaternion b, float t);

// Linearly interpolates two rotations
// Is faster but does not produce the same result as SLERP
quaternion quat_lerp(quaternion a, quaternion b, float t);
#endif