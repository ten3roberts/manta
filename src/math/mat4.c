#define _USE_MATH_DEFINES
#include <stdint.h>
#include <math.h>
#include "math/vec.h"
#include "math/mat4.h"
#include <stdio.h>

// Returns a 4x4 matrix initialized with zero
const mat4 mat4_zero = {{{0, 0, 0, 0},

						 {0, 0, 0, 0},

						 {0, 0, 0, 0},

						 {0, 0, 0, 0}}};

// Returns a 4x4 identity matrix
const mat4 mat4_identity = {{{1, 0, 0, 0},

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
			if (i == 3 && j != 3)
				result.raw[i][j] = *(&v.x + j);
			else if (i == j)
				result.raw[i][j] = 1;
			else
				result.raw[i][j] = 0;
		}
	}
	return result;
}

mat4 mat4_scale(vec3 v)
{
	mat4 result;
	for (size_t i = 0; i < 4; i++)
	{
		for (size_t j = 0; j < 4; j++)
		{
			if (i == j && i < 3)
				result.raw[i][j] = *(&v.x + i);
			else if (i == j)
				result.raw[i][j] = 1;
			else
				result.raw[i][j] = 0;
		}
	}
	return result;
}

mat4 mat4_perspective(float aspect, float fov, float near, float far)
{
	mat4 result		 = mat4_identity;
	result.raw[0][0] = 1 / (aspect * tan(fov / 2));
	result.raw[1][1] = 1 / tan(fov / 2);
	result.raw[2][2] = -(far + near) / (far - near);
	result.raw[3][2] = -2 * (far * near) / (far - near);
	result.raw[2][3] = -1;
	return result;
}

mat4 mat4_ortho(float width, float height, float near, float far)
{
	mat4 result		 = mat4_identity;
	result.raw[0][0] = 2 / (width);
	result.raw[1][1] = 2 / (height);
	result.raw[2][2] = -2 / (far - near);
	result.raw[2][3] = -(far + near) / (far - near);
	result.raw[3][3] = 1;
	return result;
}

mat4 mat4_mul(const mat4* a, const mat4* b)
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
mat4 mat4_scalar_mul(const mat4* a, float scalar)
{
	mat4 result;
	for (uint8_t i = 0; i < 16; i++)
	{
		*(&result.raw[0][0] + i) = *(&a->raw[0][0] + i) * scalar;
	}
	return result;
}

mat4 mat4_add(const mat4* a, const mat4* b)
{
	mat4 result = mat4_zero;
	for (uint8_t i = 0; i < 4; i++)
		for (uint8_t j = 0; j < 4; j++)
			result.raw[i][j] = a->raw[i][j] + b->raw[i][j];
	return result;
}

mat4 mat4_transpose(const mat4* a)
{
	mat4 result = mat4_zero;
	for (uint8_t i = 0; i < 4; i++)
		for (uint8_t j = 0; j < 4; j++)
			result.raw[j][i] = a->raw[i][j];
	return result;
}

static float determinant_internal(const float matrix[4][4], int n)
{
	float det = 0;
	float submatrix[4][4];
	if (n == 2)
		return ((matrix[0][0] * matrix[1][1]) - (matrix[1][0] * matrix[0][1]));
	else
	{
		for (int x = 0; x < n; x++)
		{
			int subi = 0;
			for (int i = 1; i < n; i++)
			{
				int subj = 0;
				for (int j = 0; j < n; j++)
				{
					if (j == x)
						continue;
					submatrix[subi][subj] = matrix[i][j];
					subj++;
				}
				subi++;
			}
			det = det + (pow(-1, x) * matrix[0][x] * determinant_internal(submatrix, n - 1));
		}
	}
	return det;
}

float mat4_determinant(const mat4* a)
{
	return determinant_internal(a->raw, 4);
}

static float invf(int i, int j, const float* m)
{

	int o = 2 + (j - i);

	i += 4 + o;
	j += 4 - o;

#define e(a, b) m[((j + b) % 4) * 4 + ((i + a) % 4)]

	float inv = +e(+1, -1) * e(+0, +0) * e(-1, +1) + e(+1, +1) * e(+0, -1) * e(-1, +0) + e(-1, -1) * e(+1, +0) * e(+0, +1) -
				e(-1, -1) * e(+0, +0) * e(+1, +1) - e(-1, +1) * e(+0, -1) * e(+1, +0) - e(+1, -1) * e(-1, +0) * e(+0, +1);

	return (o % 2) ? inv : -inv;

#undef e
}

mat4 mat4_inverse(const mat4* a)
{
	// Take matrix and convert to in line represenatation
	float* m = (float*)a;
	float inv[16];

	for (int i = 0; i < 4; i++)
		for (int j = 0; j < 4; j++)
			inv[j * 4 + i] = invf(i, j, m);

	double D = 0;

	for (int k = 0; k < 4; k++)
		D += m[k] * inv[k * 4];

	if (D == 0)
		return mat4_zero;

	D = 1.0 / D;

	for (int i = 0; i < 16; i++)
		inv[i] = inv[i] * D;

	return *(mat4*)(&inv);
}

// Performs a matrix vector column multiplication
vec4 mat4_transform_vec4(const mat4* m, vec4 v)
{
	return (vec4){m->raw[0][0] * v.x + m->raw[1][0] * v.y + m->raw[2][0] * v.z + m->raw[3][0] * v.w,
				  m->raw[0][1] * v.x + m->raw[1][1] * v.y + m->raw[2][1] * v.z + m->raw[3][1] * v.w,
				  m->raw[0][2] * v.x + m->raw[1][2] * v.y + m->raw[2][2] * v.z + m->raw[3][2] * v.w,
				  m->raw[0][3] * v.x + m->raw[1][3] * v.y + m->raw[2][3] * v.z + m->raw[3][3] * v.w};
}

// Performs a matrix vector column multiplication
vec3 mat4_transform_vec3(const mat4* m, vec3 v)
{
	return (vec3){m->raw[0][0] * v.x + m->raw[1][0] * v.y + m->raw[2][0] * v.z + m->raw[3][0],
				  m->raw[0][1] * v.x + m->raw[1][1] * v.y + m->raw[2][1] * v.z + m->raw[3][1],
				  m->raw[0][2] * v.x + m->raw[1][2] * v.y + m->raw[2][2] * v.z + m->raw[3][2]};
}

void mat4_string(mat4* a, char* buf, int precision)
{
	for (uint8_t i = 0; i < 4; i++)
	{
		for (uint8_t j = 0; j < 4; j++)
		{
			buf += ftos_pad(a->raw[i][j], buf, precision - 1, precision + 2, ' ');

			*buf++ = ',';
			*buf++ = ' ';
		}
		*buf++ = '\n';
	}
	*buf++ = '\0';
}