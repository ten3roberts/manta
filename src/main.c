#include "math/mat4.h"
#include "math/vec3.h"
#include "utils.h"
#include <stdio.h>

void mat4_print(mat4 * a)
{
	for (uint8 i = 0; i < 4; i++)
		printf("%f, %f, %f, %f\n", a->raw[i][0], a->raw[i][1], a->raw[i][2], a->raw[i][3]);
}

int main(int argc, char ** argv)
{
	char buf[100];
	dir_up(argv[0], buf, sizeof buf, 2);
	mat4 a = mat4_translate((vec3){0, 1, 0});

	vec3 v = {1, 1, 1};
	v = mat4_vec3_mul(&a, v);
	printf("%f, %f, %f\n", v.x, v.y, v.z);
}