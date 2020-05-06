#include "string.h"
#include "application.h"
#include "keycodes.h"
#include "log.h"
#include <GLFW/glfw3.h>
#include <stdbool.h>
#include <stdio.h>
#include "magpie.h"
#include <string.h>

#include <GLFW/glfw3.h>
#include "stb_image.h"

#define WS_WINDOWED	  1
#define WS_BORDERLESS 2
#define WS_FULLSCREEN 4

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
	application_send_event((Event){EVENT_WINDOW_RESIZE, .idata = {width, height}, 0});
}

void window_focus_callback(GLFWwindow* raw_window, int focus)
{
	Window* window = glfwGetWindowUserPointer(raw_window);
	window->in_focus = focus;
	application_send_event((Event){EVENT_WINDOW_FOCUS, .idata = {focus, 0}, 0});
}

void window_close_callback(GLFWwindow* raw_window)
{
	Window* window = glfwGetWindowUserPointer(raw_window);
	window->should_close = 1;
	application_send_event((Event){EVENT_WINDOW_CLOSE, .idata = {1, 0}, 0});
}

void key_callback(GLFWwindow* raw_window, int key, int scancode, int action, int mods)
{
	switch (action)
	{
	case GLFW_PRESS: {
		application_send_event((Event){EVENT_KEY, .idata = {key, 1}, 0});
		break;
	}
	case GLFW_RELEASE: {
		application_send_event((Event){EVENT_KEY, .idata = {key, 0}, 0});
		break;
	}
	case GLFW_REPEAT: {
		application_send_event((Event){EVENT_KEY, .idata = {key, 1}, 0});
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
		application_send_event((Event){EVENT_KEY, .idata = {MOUSE_1 + key, 1}, 0});
		break;
	}
	case GLFW_RELEASE: {
		application_send_event((Event){EVENT_KEY, .idata = {MOUSE_1 + key, 0}, 0});
		break;
	}
	default:
		break;
	}
}

void scroll_callback(GLFWwindow* raw_window, double xScroll, double yScroll)
{
	application_send_event((Event){EVENT_MOUSE_SCROLLED, .fdata = {xScroll, yScroll}, 0});
}

void mouse_moved_callback(GLFWwindow* raw_window, double x, double y)
{
	application_send_event((Event){EVENT_MOUSE_MOVED, .fdata = {x, y}, 0});
}

Window* window_create(char* title, int width, int height, int style, int resizable)
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
	glfwWindowHint(GLFW_RESIZABLE, resizable);

	LOG("Creating window %d %d", width, height);
	Window* window = malloc(sizeof(Window));
	if (window == NULL)
	{
		return NULL;
	}
	window->width = width;
	window->height = height;
	window->in_focus = 1;
	window->should_close = 0;
	snprintf(window->title, sizeof window->title, "%s", title);

	if (style == 0)
	{
		LOG_E("Error empty window style");
	}
	if (style & WS_WINDOWED)
	{
		window->raw_window = glfwCreateWindow(window->width, window->height, window->title, NULL, NULL);
	}

	else if (style & WS_BORDERLESS)
	{
		// const GLFWvidmode* mode = glfwGetVideoMode(primary);
		glfwWindowHint(GLFW_RED_BITS, mode->redBits);
		glfwWindowHint(GLFW_GREEN_BITS, mode->greenBits);
		glfwWindowHint(GLFW_BLUE_BITS, mode->blueBits);
		glfwWindowHint(GLFW_REFRESH_RATE, mode->refreshRate);
		glfwWindowHint(GLFW_DECORATED, GL_FALSE);

		window->raw_window = glfwCreateWindow(window->width, window->height, window->title, NULL, NULL);
	}

	else if (style & WS_FULLSCREEN)
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

int window_set_icon(Window* window, const char* small, const char* large)
{
	GLFWimage images[2]; //small,large
	int channels = 0;
	if ((images[0].pixels = stbi_load(small, &images[0].width, &images[0].height, &channels, STBI_rgb_alpha)) == NULL)
	{
		LOG_W("Failed to load small window icon %s", small);
		return EXIT_FAILURE;
	}
	if ((images[1].pixels = stbi_load(large, &images[1].width, &images[1].height, &channels, STBI_rgb_alpha)) == NULL)
	{
		LOG_W("Failed to load large window icon %s", large);
		return EXIT_FAILURE;
	}

	// Only supply one if either is NULL
	GLFWimage* pimages = NULL;
	int image_count = (small != NULL) + (large != NULL);
	if (small)
	{
		pimages = images;
	}
	else if (large)
	{
		pimages = images + 1;
	}

	glfwSetWindowIcon(window->raw_window, image_count, pimages);
	stbi_image_free(images[0].pixels);
	stbi_image_free(images[1].pixels);
	return EXIT_SUCCESS;
}

void window_destroy(Window* window)
{
	glfwDestroyWindow(window->raw_window);
	window->raw_window = NULL;
	window_count--;
	if (window_count == 0)
		glfwTerminate();
	free(window);
}

void window_update(Window* window)
{
	glfwPollEvents();
}

int window_get_width(Window* window)
{
	return window->width;
}
int window_get_height(Window* window)
{
	return window->height;
}

float window_get_aspect(Window* window)
{
	return window->width / (float)window->height;
}
int window_get_minimized(Window* window)
{
	return window->width == 0 || window->height == 0;
}

int window_get_close(Window* window)
{
	return window->should_close;
}

void* window_get_raw(Window* window)
{
	return window->raw_window;
}