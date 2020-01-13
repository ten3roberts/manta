#pragma once
#include <GLFW/glfw3.h>
typedef enum
{
	// Creates a decorated window. Uses set width and height
	WS_WINDOWED,
	// Creates an undercorated window. Uses set width and height
	WS_BORDERLESS,
	// Creates a fullscreen undecorated window covering panels and status bars
	// // Width and height are set to native resolution
	WS_FULLSCREEN
} WindowStyle;

typedef void Window;

extern Window * window_create(char * title, int width, int height, WindowStyle style);

// Destroys a window and frees all resources
extern void window_destroy(Window * window);

extern void window_update(Window * window);

// Returns true if the window should close
extern int window_get_close(Window * window);