#include "math/mat4.h"
#include "math/quaternion.h"
#include "math/vec3.h"
#include "utils.h"
#include "window.h"
#include <stdio.h>
#include "log.h"
#include <string.h>
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
	log_init();
	// Changes the working directory to be consistent no matter how the application is started
	{
		char buf[2048];
		dir_up(argv[0], buf, sizeof buf, 2);
	}

	int a = 0x5;
	int b = 0x1056;

	LOG("TEST %s", "Hello");
	Window* window = window_create("crescent", 800, 600);

	while (!window_get_close(window))
	{
		window_update(window);
	}
	window_destroy(window);
	log_terminate();
}