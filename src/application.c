#include "application.h"
#include "cr_time.h"
#include "log.h"
#include "math/math_extra.h"
#include "window.h"
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include "math/vec.h"
#include "input.h"
#include "graphics/vulkan.h"
#include "graphics/swapchain.h"
#include "timer.h"
#include "graphics/renderer.h"
#include "settings.h"

Window* main_window = NULL;

void draw()
{
	renderer_draw();
}

int swapchain_resize = 0;
int application_start()
{
	Timer timer = timer_start(CT_WALL_TICKS);
	if (DEBUG)
		LOG_S("Running in debug mode");
	time_init();

	settings_load();

	main_window = window_create("crescent", settings_get_resolution().x,  settings_get_resolution().y, settings_get_window_style());
	if (main_window == NULL)
		return -1;

	input_init(main_window);
	vulkan_init();
	LOG_S("Initialization took %f ms", timer_stop(&timer) * 1000);

	timer_reset(&timer);
	swapchain_resize = 0;
	while (!window_get_close(main_window))
	{
		if (window_get_minimized(main_window))
		{
			SLEEP(0.1);
		}
		input_update();

		// Don't resize every frame
		if (swapchain_resize == 2)
			swapchain_resize = 1;
		window_update(main_window);
		if (swapchain_resize == 1)
		{
			swapchain_recreate();
			swapchain_resize = 0;
		}
		else if(swapchain_resize == 2)
		{
			continue;
		}
		time_update();
		draw();
		if (timer_duration(&timer) > 1.0f)
		{
			timer_reset(&timer);
			LOG("Framerate %d %f", time_framecount(), time_framerate());
		}
	}
	vulkan_terminate();
	LOG_S("Terminating");
	window_destroy(main_window);
	settings_save();

	return 0;
}


void application_send_event(Event event)
{
	// Recreate swapchain on resize
	if (event.type == EVENT_WINDOW_RESIZE && event.idata[0] != 0 && event.idata[1] != 0)
	{
		LOG("%d %d", event.idata[0], event.idata[1]);
		swapchain_resize = 2;
	}
	if (event.type == EVENT_KEY)
		LOG("Key pressed  : %d, %c", event.idata[0], event.idata[0]);
	if (event.type == EVENT_KEY || event.type == EVENT_MOUSE_MOVED || event.type == EVENT_MOUSE_SCROLLED)
		input_send_event(&event);
}

void* application_get_window()
{
	return main_window;
}