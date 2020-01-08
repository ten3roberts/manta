#include "math/mat4.h"
#include "math/quaternion.h"
#include "math/vec3.h"
#include "utils.h"
#include <windows.h>
#include <stdio.h>

void mat4_print(mat4* a)
{
	for (uint8 i = 0; i < 4; i++)
		printf("%f, %f, %f, %f\n", a->raw[i][0], a->raw[i][1], a->raw[i][2], a->raw[i][3]);
}

int main(int argc, char** argv)
{
	// Changes the working directory to be consistent no matter how the application is started
	{
		char buf[2048];
		dir_up(argv[0], buf, sizeof buf, 2);
	}

	quaternion q = quat_axis_angle((vec3) { 1, 0, 0 }, 1);
	q = quat_ang_scale(q, 2);
	while (1);
}