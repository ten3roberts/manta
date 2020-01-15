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

int application_start()
{
	time_init();
	input_init();

	Window * window = window_create("crescent", 800, 600, WS_WINDOWED);
	if (window == NULL)
		return -1;

	LOG("%f", 350.1f);
	LOG_OK("TEST");

	while (!window_get_close(window))
	{
		input_update();
		window_update(window);
		LOG("%d", input_getkey(CR_MOUSE_1));
		LOG("%f", time_elapsed());
		time_update();
		SLEEP(0.1f);
	}

	window_destroy(window);

	return 0;
}

void application_send_event(Event event)
{
	if (event.type == EVENT_MOUSE_MOVED)
		return;
	/*else if (event.type == EVENT_KEY_PRESSED)
		LOG("Key pressed  : %d, %c", event.idata[0], event.idata[0]);
	else if (event.type == EVENT_KEY_RELEASED)
		LOG("Key released : %d, %c", event.idata[0], event.idata[0]);
	else
		LOG("Event type %d : %f, %f", event.type, event.fdata[0], event.fdata[1]);*/
	if (event.type == EVENT_KEY)
		input_send_event(&event);
}