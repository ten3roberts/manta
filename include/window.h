#ifndef WINDOW_H
#define WINDOW_H

#define WS_WINDOWED	  1
#define WS_BORDERLESS 2
#define WS_FULLSCREEN 4

typedef void Window;

Window* window_create(char* title, int width, int height, int style, int resizable);

// Sets the window icon from an image file
// Chooses between the small and large versions of the image, either one can be NULL
// Returns EXIT_SUCCESS on success
int window_set_icon(Window* window, const char* small, const char* large);

// Destroys a window and frees all resources
void window_destroy(Window* window);


void window_update(Window* window);

int window_get_width(Window* window);
int window_get_height(Window* window);

// Returns the window aspect ratio calculated by width/height
float window_get_aspect(Window* window);

// Returns true if the window should close
int window_get_close(Window* window);

int window_get_minimized(Window* window);

// Returns the raw glfw window
// Returns as void* so that glfw3.h does not need to be included in header
void* window_get_raw(Window* window);
#endif
