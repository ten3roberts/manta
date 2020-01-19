#pragma once
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

extern float window_get_width(Window * window);
extern float window_get_height(Window * window);

// Returns true if the window should close
extern int window_get_close(Window * window);

// Returns the raw glfw window
// Returns as void* so that glfw3.h does not need to be included in header
extern void* window_get_raw(Window* window);
