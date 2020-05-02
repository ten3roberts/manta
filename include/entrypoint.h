#ifndef ENTRYPOINT_H
#define ENTRYPOINT_H

#include <stdio.h>
#include "magpie.h"
#include "log.h"
#include <string.h>
#include "application.h"
#include "utils.h"

extern int application_start();

int main(int argc, char** argv)
{
	// Changes the working directory to be consistent no matter how the application is started
	{
		char buf[2048];
		dir_up(argv[0], buf, sizeof buf, 2);
		set_workingdir(buf);
	}

	log_init();

#ifdef DEBUG
	LOG_S("Running in debug mode");
#endif

	application_start(argc, argv);

// Include allocation information on debug builds
#ifdef DEBUG
	mp_print_locations();
	LOG("A total of %d allocations were made", mp_get_total_count());
	//mp_print_locations();
	mp_terminate();
#endif
	log_terminate();
	return 0;
}
#endif
