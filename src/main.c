#include <stdio.h>
#include "log.h"
#include <string.h>
#include "application.h"
#include "utils.h"

int main(int argc, char** argv)
{
	// Changes the working directory to be consistent no matter how the application is started
	{
		char buf[2048];
		dir_up(argv[0], buf, sizeof buf, 2);
		set_workingdir(buf);
	}

	log_init();

	application_start();

	log_terminate();
	return 0;
}