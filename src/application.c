#include "application.h"
#include "cr_time.h"
#include "log.h"
#include "math/math_extra.h"
#include "window.h"
#include <time.h>
#include <unistd.h>
#include <stdio.h>

int application_start()
{
	time_init();

	// Window * window = window_create("crescent", 800, 600, WS_WINDOWED);
	// if (window == NULL)
	//	return -1;
	LOG("Test %s", "string");
	LOG("%f", 1.59999f);
	int* a = malloc(sizeof (int));
	LOG("%p", a);
	printf("%p\n", a);
	while (1)
	{
		time_update();
		// window_update(window);
		// LOG("Elapsed time %f", time_elapsed());
		struct timespec start, end;

		SLEEP(0.1f);
	}

	// window_destroy(window);

	return 0;
}
