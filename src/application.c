#include "application.h"
#include "cr_time.h"
#include "log.h"
#include "math/math_extra.h"
#include "window.h"
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#include "math/vec.h"

int application_start()
{
	time_init();

	Window * window = window_create("crescent", 800, 600, WS_WINDOWED);
	if (window == NULL)
		return -1;

	
	char buf[1024];
	vec4 v = {0.3497,2,3,4};
	LOG("%4v", v);
	LOG("%f", 0.01);
	LOG_OK("TEST", 4);
	
	while (1)
	{
		time_update();
		window_update(window);
		LOG("Elapsed time %f", time_elapsed());

		SLEEP(1.0f);
	}

	window_destroy(window);

	return 0;
}
