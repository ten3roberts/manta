#include "math/vec.h"

typedef enum
{
	VSYNC_NONE,
	VSYNC_DOUBLE,
	VSYNC_TRIPLE
} VsyncMode;

void settings_load();
void settings_save();

ivec2 settings_get_resolution();
int settings_get_window_style();
VsyncMode settings_get_vsync();
int settings_get_msaa();

void settings_set_resolution(ivec2 res);
void settings_set_window_style(int ws);
void settings_set_vsync(VsyncMode mode);
void settings_set_msaa(int samples);