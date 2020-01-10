#include <stdio.h>
#include "log.h"
#include <string.h>
#include "application.h"
#include <utils.h>

int main(int argc, char** argv)
{
	log_init();

	// Changes the working directory to be consistent no matter how the application is started
	{
		char buf[2048];
		dir_up(argv[0], buf, sizeof buf, 2);
	}

	application_start();

	log_terminate();
}