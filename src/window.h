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

Window window_create(char* title, int width, int height);

/*void window_destroy(Window window);

void window_update(Window window);*/
