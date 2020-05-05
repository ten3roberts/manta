#ifndef MAT4_H
#define MAT4_H
#include "vec.h"
#include <stdint.h>

// [row][col]
typedef struct
{
	float raw[4][4];

} mat4;

// Returns a 4x4 matrix initialized with zero
extern const mat4 mat4_zero;
// Returns a 4x4 identity matrix
extern const mat4 mat4_identity;

// Returns a translation matrix
mat4 mat4_translate(vec3 v);

//Returns a scaling matrix
mat4 mat4_scale(vec3 v);

// Generates a perspective projection matrix
mat4 mat4_perspective(float aspect, float fov, float near, float far);

// Generates a orthographic projection matrix
// Such that if width is window aspect and height is 1, a 2 units wide quad will cover the screen
// Maps the coordinates to x : {-width, width} and y = {-height, height}
mat4 mat4_ortho(float width, float height, float near, float far);

mat4 mat4_mul(const mat4* a, const mat4* b);

// Multiplies a scalar pairwise on the matrix
mat4 mat4_scalar_mul(const mat4* a, float scalar);

mat4 mat4_add(const mat4* a, const mat4* b);

mat4 mat4_transpose(const mat4* a);

float mat4_determinant(const mat4* a);

mat4 mat4_inverse(const mat4* a);

// Performs a matrix vector column multiplication
vec4 mat4_transform_vec4(const mat4* m, vec4 v);

// Performs a matrix vector column multiplication
vec3 mat4_transform_vec3(const mat4* m, vec3 v);

// Converts a matrix to a comma line separated float grid
void mat4_string(mat4* a, char* buf, int precision);
#endif