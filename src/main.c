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
	char buf[128];
	dir_up(argv[0], buf, sizeof buf, 2);
	puts(buf);
	dir_up("./a/b/", buf, sizeof buf, 1);
	puts(buf);
	while (1);
}