#ifndef SETTINGS_H
#define SETTINGS_H
#include "math/vec.h"

enum VsyncMode
{
	VSYNC_NONE,
	VSYNC_DOUBLE,
	VSYNC_TRIPLE
};

void settings_load();
void settings_save();

ivec2 settings_get_resolution();
int settings_get_window_style();
enum VsyncMode settings_get_vsync();
int settings_get_msaa();

void settings_set_resolution(ivec2 res);
void settings_set_window_style(int ws);
void settings_set_vsync(enum VsyncMode mode);
void settings_set_msaa(int samples);
#endif