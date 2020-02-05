#pragma once
#define _USE_MATH_DEFINES
#include <stdint.h>
#include <math.h>
#include "vec.h"

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
mat4 mat4_translate(vec3 v)
{
	mat4 result;
	for (size_t i = 0; i < 4; i++)
	{
		for (size_t j = 0; j < 4; j++)
		{
			if (j == 3 && i != 3)
				result.raw[i][j] = *(&v.x + i);
			else if (i == j)
				result.raw[i][j] = 1;
			else
				result.raw[i][j] = 0;
		}
	}
	return result;
}

mat4 mat4_mul(const mat4 * a, const mat4 * b)
{
	mat4 result = mat4_zero;
	for (uint8_t i = 0; i < 4; i++) // row
	{
		for (uint8_t j = 0; j < 4; j++) // col
		{
			// Loops through second matrix column and puts in result col
			for (uint8_t d = 0; d < 4; d++)
				result.raw[i][d] += a->raw[i][j] * b->raw[j][d];
		}
	}
	return result;
}

// Multiplies a scalar pairwise on the matrix
mat4 mat4_scalar_mul(const mat4 * a, float scalar)
{
	mat4 result;
	for (uint8_t i = 0; i < 16; i++)
	{
		*(&result.raw[0][0] + i) = *(&a->raw[0][0] + i) * scalar;
	}
	return result;
}

mat4 mat4_add(const mat4 * a, const mat4 * b)
{
	mat4 result = mat4_zero;
	for (uint8_t i = 0; i < 4; i++)
		for (uint8_t j = 0; j < 4; j++)
			result.raw[i][j] = a->raw[i][j] + b->raw[i][j];
	return result;
}

mat4 mat4_transpose(const mat4 * a)
{
	mat4 result = mat4_zero;
	for (uint8_t i = 0; i < 4; i++)
		for (uint8_t j = 0; j < 4; j++)
			result.raw[j][i] = a->raw[i][j];
	return result;
}

// Performs a matrix vector column multiplication
vec4 mat4_vec4_mul(const mat4 * m, vec4 v)
{
	return (vec4){m->raw[0][0] * v.x + m->raw[0][1] * v.y + m->raw[0][2] * v.z + m->raw[0][3] * v.w,
				  m->raw[1][0] * v.x + m->raw[1][1] * v.y + m->raw[1][2] * v.z + m->raw[1][3] * v.w,
				  m->raw[2][0] * v.x + m->raw[2][1] * v.y + m->raw[2][2] * v.z + m->raw[2][3] * v.w,
				  m->raw[3][0] * v.x + m->raw[3][1] * v.y + m->raw[3][2] * v.z + m->raw[3][3] * v.w};
}

// Performs a matrix vector column multiplication
vec3 mat4_vec3_mul(const mat4 * m, vec3 v)
{
	return (vec3){m->raw[0][0] * v.x + m->raw[0][1] * v.y + m->raw[0][2] * v.z + m->raw[0][3],
				  m->raw[1][0] * v.x + m->raw[1][1] * v.y + m->raw[1][2] * v.z + m->raw[1][3],
				  m->raw[2][0] * v.x + m->raw[2][1] * v.y + m->raw[2][2] * v.z + m->raw[2][3]};
}