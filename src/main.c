#include "math/mat4.h"
#include "math/quaternion.h"
#include "math/vec3.h"
#include "utils.h"
#include "window.h"
#include <stdio.h>

void mat4_print(mat4* a)
{
	for (uint8 i = 0; i < 4; i++)
		printf("%f, %f, %f, %f\n", a->raw[i][0], a->raw[i][1], a->raw[i][2], a->raw[i][3]);
}

struct my_struct
{
	vec3 v;
	float w;
};

int main(int argc, char** argv)
{
	// Changes the working directory to be consistent no matter how the application is started
	{
		char buf[2048];
		dir_up(argv[0], buf, sizeof buf, 2);
	}
	vec4 a = { 1,2,3,4 };

	//printf("%f %f %f", b.x, b.y, b.z);
	quaternion q = quat_axis_angle((vec3) { 1, 0, 0 }, 1);
	q = quat_ang_scale(q, 2);

	Window* window = window_create("crescent", 800, 600);

	while (!window_get_close(window))
	{
		window_update(window);
		printf("%c", rand() % 40 + 65);
		if (rand() % 5 == 0)
			printf(" ");
		if (rand() % 64 == 0)
			printf("\n");
	}
	window_destroy(window);
}