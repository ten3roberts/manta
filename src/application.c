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
#include "timer.h"
#include "graphics/renderer.h"

Window* window = NULL;

void draw()
{
	renderer_draw();
}

int application_start()
{
	Timer timer = timer_start();
	time_init();

	window = window_create("crescent", 800, 600, WS_WINDOWED);
	if (window == NULL)
		return -1;

	input_init(window);
	vulkan_init();
	LOG_S("Initialization took %f ms", timer_stop(&timer) * 1000);

	while (!window_get_close(window))
	{
		input_update();
		window_update(window);
		time_update();
		draw();
		SLEEP(0.05f);
	}
	LOG_S("Terminating");
	vulkan_terminate();
	window_destroy(window);

	return 0;
}

void application_send_event(Event event)
{
	if (event.type == EVENT_KEY)
		LOG("Key pressed  : %d, %c", event.idata[0], event.idata[0]);
	if (event.type == EVENT_KEY || event.type == EVENT_MOUSE_MOVED || event.type == EVENT_MOUSE_SCROLLED)
		input_send_event(&event);
}

void* application_get_window()
{
	return window;
}