#include "settings.h"
#include "log.h"
#include "window.h"
#include <stdio.h>
#include <stdlib.h>


ivec2 resolution = {800, 600};
int window_style = WS_WINDOWED;
enum VsyncMode vsync = VSYNC_NONE;
int msaa = 1;

#define STRING(s) #s

void settings_load()
{
	FILE* file = NULL;
	file = fopen("./assets/settings", "r");
	if (file == NULL)
	{
		LOG_E("Failed to open settings file");
		return;
	}
	char buf[2048];
	while (fgets(buf, sizeof buf, file))
	{
		if (strcmp(buf, "\n") == 0)
		{
			LOG("Empty");
			continue;
		}
		strtok(buf, "\n");
		char lh[2056];
		char* space = strchr(buf, ' ');
		strncpy(lh, buf, space - buf);
		lh[space - buf] = '\0';
		char* rh = space + 1;

		if (strcmp(lh, "resolution") == 0)
		{
			sscanf(rh, "%d,%d", &resolution.x, &resolution.y);
		}
		else if (strcmp(lh, "window_style") == 0)
		{
			window_style = atoi(rh);
		}
		else if (strcmp(lh, "vsync") == 0)
		{
			vsync = atoi(rh);
		}
		else if (strcmp(lh, "msaa") == 0)
		{
			msaa = atoi(rh);
		}
	}
	fclose(file);
}

void settings_save()
{
	FILE* file = NULL;
	file = fopen("./assets/settings", "w");
	if (file == NULL)
	{
		LOG_E("Failed to open settings file");
		return;
	}

	fprintf(file, "resolution %d,%d\n", resolution.x, resolution.y);
	fprintf(file, "window_style %d\n", window_style);
	fprintf(file, "vsync %d\n", vsync);
	fprintf(file, "msaa %d\n", msaa);

	fclose(file);
}

ivec2 settings_get_resolution()
{
	return resolution;
}

int settings_get_window_style()
{
	return window_style;
}

enum VsyncMode settings_get_vsync()
{
	return vsync;
}
int settings_get_msaa()
{
	return msaa;
}

void settings_set_resolution(ivec2 res)
{
	resolution = res;
}
void settings_set_window_style(int ws)
{
	window_style = ws;
}
void settings_set_vsync(enum VsyncMode mode)
{
	vsync = mode;
}
