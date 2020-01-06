#include "math/mat4.h"
#include "math/quaternion.h"
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

	// quaternion q = quat_axis_angle((vec3){1,1,0}, 2);
	quaternion q = quat_axis_angle((vec3){0, 1, 0}, DEG_90);
	printf("%f, %f, %f, %f\n", q.x, q.y, q.z, q.w);
	vec3 v = quat_vec3_mul(q, vec3_forward);
	printf("%f %f %f,\n", v.x, v.y, v.z);
	v = quat_vec3_mul(q, vec3_right);
	printf("%f %f %f,\n", v.x, v.y, v.z);
}