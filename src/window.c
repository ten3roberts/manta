#include "string.h"
#include "application.h"
#include "keycodes.h"
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
	GLFWwindow* raw_window;
} Window;

int glfw_initialized = 0;
size_t window_count = 0;

void window_size_callback(GLFWwindow* raw_window, int width, int height)
{
	Window* window = glfwGetWindowUserPointer(raw_window);
	window->width = width;
	window->height = height;
	application_send_event((Event) { EVENT_WINDOW_SIZE, .idata = { width, height } });
}

void window_focus_callback(GLFWwindow* raw_window, int focus)
{
	Window* window = glfwGetWindowUserPointer(raw_window);
	window->in_focus = focus;
	application_send_event((Event) { EVENT_WINDOW_FOCUS, .idata = { focus, 0 } });
}

void window_close_callback(GLFWwindow* raw_window)
{
	Window* window = glfwGetWindowUserPointer(raw_window);
	window->should_close = 1;
	application_send_event((Event) { EVENT_WINDOW_CLOSE, .idata = { 1, 0 } });
}

void key_callback(GLFWwindow* raw_window, int key, int scancode, int action, int mods)
{
	Window* window = glfwGetWindowUserPointer(raw_window);
	switch (action)
	{
	case GLFW_PRESS: {
		application_send_event((Event) { EVENT_KEY_PRESSED, .idata = { key, 0 } });
		break;
	}
	case GLFW_RELEASE: {
		application_send_event((Event) { EVENT_KEY_RELEASED, .idata = { key, 0 } });
		break;
	}
	case GLFW_REPEAT: {
		application_send_event((Event) { EVENT_KEY_PRESSED, .idata = { key, 1 } });
		break;
	}
	default:
		break;
	}
}

void mouse_button_callback(GLFWwindow* raw_window, int key, int action, int mods)
{
	switch (action)
	{
	case GLFW_PRESS: {
		application_send_event((Event) { EVENT_KEY_PRESSED, .idata = { CR_MOUSE_1 + key, 0 } });
		break;
	}
	case GLFW_RELEASE: {
		application_send_event((Event) { EVENT_KEY_RELEASED, .idata = { CR_MOUSE_1 + key, 0 } });
		break;
	}
	default:
		break;
	}
}

void scroll_callback(GLFWwindow* raw_window, double xScroll, double yScroll)
{
	application_send_event((Event) { EVENT_MOUSE_SCROLLED, .fdata = { xScroll, yScroll } });
}

void mouse_moved_callback(GLFWwindow* raw_window, double x, double y)
{
	application_send_event((Event) { EVENT_MOUSE_MOVED, .fdata = { x, y } });
}

Window* window_create(char* title, int width, int height, WindowStyle style)
{
	// Make sure to initialize glfw once
	if (!glfw_initialized && glfwInit())
		glfw_initialized = 1;

	// Sets the resolution to native if res is set to -1
	GLFWmonitor* primary = glfwGetPrimaryMonitor();
	const GLFWvidmode* mode = glfwGetVideoMode(primary);
	width = width > 0 ? width : mode->width;
	height = height > 0 ? height : mode->height;

	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
	glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

	printf("Creating window\n");
	Window* window = malloc(sizeof(Window));
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

	glfwSetWindowUserPointer(window->raw_window, window);
	// Set up event handling
	// glfwSetWindowSizeCallback(window->raw_window, [](GLFWwindow * window, int width, int height) {
	glfwSetWindowSizeCallback(window->raw_window, window_size_callback);

	glfwSetWindowFocusCallback(window->raw_window, window_focus_callback);
	glfwSetWindowCloseCallback(window->raw_window, window_close_callback);

	glfwSetKeyCallback(window->raw_window, key_callback);

	glfwSetMouseButtonCallback(window->raw_window, mouse_button_callback);

	glfwSetScrollCallback(window->raw_window, scroll_callback);

	glfwSetCursorPosCallback(window->raw_window, mouse_moved_callback);
	window_count++;
	return window;
}

void window_destroy(Window* window)
{
	glfwDestroyWindow(window->raw_window);
	window_count--;
	if (window_count == 0)
		glfwTerminate();
	free(window);
}

void window_update(Window* window)
{
	glfwPollEvents();
}

bool window_get_close(Window* window)
{
	return window->should_close;
}
