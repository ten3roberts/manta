#pragma once
#include "vec.h"
#include <stdint.h>

typedef struct
{
	float raw[4][4];
} mat4;

// Returns a 4x4 matrix initialized with zero
const static mat4 mat4_zero = {{{0, 0, 0, 0},

								{0, 0, 0, 0},

								{0, 0, 0, 0},

								{0, 0, 0, 0}}};

// Returns a 4x4 identity matrix
const static mat4 mat4_identity = {{{1, 0, 0, 0},

									{0, 1, 0, 0},

									{0, 0, 1, 0},

									{0, 0, 0, 1}}};

// Returns a translation matrix
mat4 mat4_translate(vec3 v);

mat4 mat4_mul(const mat4 * a, const mat4 * b);

// Multiplies a scalar pairwise on the matrix
mat4 mat4_scalar_mul(const mat4 * a, float scalar);

mat4 mat4_add(const mat4 * a, const mat4 * b);

mat4 mat4_transpose(const mat4 * a);

// Performs a matrix vector column multiplication
vec4 mat4_vec4_mul(const mat4 * m, vec4 v);

// Performs a matrix vector column multiplication
vec3 mat4_vec3_mul(const mat4 * m, vec3 v);