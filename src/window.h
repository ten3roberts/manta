#pragma once
typedef enum
{
    // Creates a decorated window. Uses set width and height
		WS_WINDOWED,
		// Creates an undercorated window. Uses set width and height
		WS_BORDERLESS,
		// Creates a fullscreen undecorated window covering kde panels. width and
		// height are overridden to the displays resulution
		WS_FULLSCREEN
}WindowStyle;

typedef void Window;

extern Window* window_create(char* title, int width, int height);

// Destroys a window and frees all resources
extern void window_destroy(Window* window);

extern void window_update(Window* window);

// Returns true if the window should close
extern bool window_get_close(Window* window);

