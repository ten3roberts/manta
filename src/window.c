#include "string.h"
#include <GLFW/glfw3.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef enum
{
	// Creates a decorated window. Uses set width and height
	WS_WINDOWED,
	// Creates an undercorated window. Uses set width and height
	WS_BORDERLESS,
	// Creates a fullscreen undecorated window covering kde panels. width and
	// height are overridden to the displays resulution
	WS_FULLSCREEN
} WindowStyle;

typedef struct
{
	char title[256];
	int width, height;
	int in_focus;
	int should_close;
	GLFWwindow * raw_window;
} Window;

int glfw_initialized = 0;
size_t window_count = 0;

Window * window_create(char * title, int width, int height, WindowStyle style)
{
	// Make sure to initialize glfw once
	if (!glfw_initialized && glfwInit())
		glfw_initialized = 1;

	// Sets the resolution to native if res is set to -1
	GLFWmonitor * primary = glfwGetPrimaryMonitor();
	const GLFWvidmode * mode = glfwGetVideoMode(primary);
	width = width > 0 ? width : mode->width;
	height = height > 0 ? height : mode->height;

	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
	glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

	printf("Creating window\n");
	Window * window = malloc(sizeof(Window));
	if (window == NULL)
	{
		return NULL;
	}
	window->width = width;
	window->height = height;
	window->in_focus = 1;
	window->should_close = 0;
	strcpy(window->title, title);

	if (style == WS_WINDOWED)
	{
		window->raw_window = glfwCreateWindow(window->width, window->height, window->title, NULL, NULL);
	}

	else if (style == WS_BORDERLESS)
	{
		// const GLFWvidmode* mode = glfwGetVideoMode(primary);
		glfwWindowHint(GLFW_RED_BITS, mode->redBits);
		glfwWindowHint(GLFW_GREEN_BITS, mode->greenBits);
		glfwWindowHint(GLFW_BLUE_BITS, mode->blueBits);
		glfwWindowHint(GLFW_REFRESH_RATE, mode->refreshRate);
		glfwWindowHint(GLFW_DECORATED, GL_FALSE);

		window->raw_window = glfwCreateWindow(window->width, window->height, window->title, NULL, NULL);
	}

	else if (style == WS_FULLSCREEN)
	{
		// const GLFWvidmode* mode = glfwGetVideoMode(primary);
		glfwWindowHint(GLFW_RED_BITS, mode->redBits);
		glfwWindowHint(GLFW_GREEN_BITS, mode->greenBits);
		glfwWindowHint(GLFW_BLUE_BITS, mode->blueBits);
		glfwWindowHint(GLFW_REFRESH_RATE, mode->refreshRate);
		glfwWindowHint(GLFW_DECORATED, GL_FALSE);
		window->width = mode->width;
		window->height = mode->height;
		window->raw_window = glfwCreateWindow(window->width, window->height, window->title, primary, NULL);
	}
	window_count++;
	return window;
}

void window_destroy(Window * window)
{
	glfwDestroyWindow(window->raw_window);
	window_count--;
	if (window_count == 0)
		glfwTerminate();
	free(window);
}

void window_update(Window * window)
{
	glfwPollEvents();
	window->should_close = glfwWindowShouldClose(window->raw_window);
}

bool window_get_close(Window * window)
{
	return window->should_close;
}
