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

Window* main_window = NULL;

void draw()
{
	renderer_draw();
}

int application_start()
{
	Timer timer = timer_start(CT_WALL_TICKS);
	if (DEBUG)
		LOG_S("Running in debug mode");
	time_init();

	main_window = window_create("crescent", 800, 600, WS_WINDOWED);
	if (main_window == NULL)
		return -1;

	input_init(main_window);
	vulkan_init();
	LOG_S("Initialization took %f ms", timer_stop(&timer) * 1000);

	timer_reset(&timer);

	while (!window_get_close(main_window))
	{
		if (window_get_minimized(main_window))
		{
			SLEEP(0.1);
		}
		input_update();
		window_update(main_window);
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

	return 0;
}

void application_send_event(Event event)
{
	// Recreate swapchain on resize
	if (event.type == EVENT_WINDOW_RESIZE && event.idata[0] != 0 && event.idata[1] != 0)
	{
		LOG("%d %d", event.idata[0], event.idata[1]);
		swapchain_recreate();
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