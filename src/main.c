#include "math/mat4.h"
#include "math/quaternion.h"
#include "math/vec3.h"
#include "utils.h"
#include <windows.h>
#include <stdio.h>


int read_directory(const char* dir, char** result, size_t size, size_t depth)
{
	char pattern[2048];
	snprintf(pattern, 2048, "%s\\*", dir);

	WIN32_FIND_DATA data;
	HANDLE hFind;

	char fname[2048];
	if ((hFind = FindFirstFileA(pattern, &data)) != INVALID_HANDLE_VALUE) {
		do {
			sprintf(fname, "%ws", data.cFileName);
			if (!strcmp(fname, ".") || !strcmp(fname, ".."))
				continue;

			char full_path[2048];
			strcpy(full_path, dir);
			if (dir[strlen(dir) - 1] != '/')
				strcat(full_path, "/");
			strcat(full_path, fname);

			if (data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
			{
				if (depth == 1)
					continue;
				size_t tmp = read_directory(full_path, result, size, depth - 1);
				result += size - tmp;
				size = tmp;
			}
			else if (result && size)
			{
				strcpy(*result, full_path);
				result++;
				size--;
			}
			else
			{
				return 0;
			}
		} while (FindNextFile(hFind, &data) != 0);
		FindClose(hFind);
	}
	return size;
}

void mat4_print(mat4* a)
{
	for (uint8 i = 0; i < 4; i++)
		printf("%f, %f, %f, %f\n", a->raw[i][0], a->raw[i][1], a->raw[i][2], a->raw[i][3]);
}

int main(int argc, char** argv)
{
	char buf[100];
	dir_up(argv[0], buf, sizeof buf, 2);

	// quaternion q = quat_axis_angle((vec3){1,1,0}, 2);
	quaternion q = quat_axis_angle((vec3) { 0, 1, 0 }, DEG_90);
	printf("%f, %f, %f, %f\n", q.x, q.y, q.z, q.w);
	vec3 v = quat_vec3_mul(q, vec3_forward);
	printf("%f %f %f,\n", v.x, v.y, v.z);
	v = quat_vec3_mul(q, vec3_right);
	printf("%f %f %f,\n", v.x, v.y, v.z);
	char* buf1[1000];
	for (size_t i = 0; i < 1000; i++)
	{
		buf1[i] = calloc(2048, 1);
	}

	listdir("./bin", buf1, 1000, 0);
	for (size_t i = 0; i < 1000; i++)
	{
		if (buf1[i][0])
			puts(buf1[i]);
	}
	strcpy(buf, "invalid_file");
	find_file("C:/Users/", buf, 100, "crescent.exe");
	puts(buf);
	puts("Done!");
	while (1);
}