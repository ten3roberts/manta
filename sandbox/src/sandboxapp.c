#include "manta.h"

static Window* window = NULL;

int swapchain_resize = 0;

int application_start(int argc, char** argv)
{
	Timer timer = timer_start(CT_WALL_TICKS);

	time_init();

	settings_load();

	window = window_create("sandbox", settings_get_resolution().x, settings_get_resolution().y, settings_get_window_style(), 1);

	if (window == NULL)
		return -1;

	input_init(window);
	graphics_init();

	LOG_S("Initialization took %f ms", timer_stop(&timer) * 1000);

	timer_reset(&timer);
	time_init();
	swapchain_resize = 0;
	while (!window_get_close(window))
	{
		// Poll window events
		window_update(window);
		renderer_begin();
		if (window_get_minimized(window))
		{
			SLEEP(0.1);
		}

		input_update();

		time_update();
		renderer_draw();

		if (timer_duration(&timer) > 2.0f)
		{
			timer_reset(&timer);
			LOG("Framerate %d %f", time_framecount(), time_framerate());
		}
	}
	graphics_terminate();
	LOG_S("Terminating");
	window_destroy(window);
	settings_save();

	return 0;
}

void application_send_event(Event event)
{
	// Recreate swapchain on resize
	if (event.type == EVENT_WINDOW_RESIZE && event.idata[0] != 0 && event.idata[1] != 0)
	{
		LOG("%d %d", event.idata[0], event.idata[1]);
		renderer_resize();
	}
	if (event.type == EVENT_KEY)
		LOG("Key pressed  : %d, %c", event.idata[0], event.idata[0]);
	if (event.type == EVENT_KEY || event.type == EVENT_MOUSE_MOVED || event.type == EVENT_MOUSE_SCROLLED)
		input_send_event(&event);
}

void* application_get_window()
{
	return window;
}
