#include "window.h"
#include <GLFW/glfw3.h>

typedef struct
{
	char title[256];
	int width, height;
	int in_focus;
	GLFWwindow * raw_window;
} _Window;

Window window_create(char* title, int width, int height)
{

}

/*void window_destroy(Window window);

void window_update(Window window);*/
