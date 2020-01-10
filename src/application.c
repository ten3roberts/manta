#include "application.h"
#include "log.h"
#include "window.h"
#include "cr_time.h"


int application_start()
{
	time_init();

	Window* window = window_create("crescent", 800, 600);
	if (window == NULL)
		return -1;

	while (!window_get_close(window))
	{
		window_update(window);
		time_update();
		LOG("Elapsed time %f", time_elapsed());

		SLEEP(0.1f);
	}

	window_destroy(window);

	return 0;
}
