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
void settings_set_resolution(ivec2 res);
int settings_get_window_style();
void settings_set_window_style(int ws);
VsyncMode settings_get_vsync();
void settings_set_vsync(VsyncMode mode);
