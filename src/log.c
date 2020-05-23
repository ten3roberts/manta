#include "log.h"
#include "cr_time.h"
#include "math/math.h"
#include "utils.h"
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include "math/mat4.h"

#include "math/vec.h"

#if PL_LINUX
void set_print_color(int color)
{
	printf("\x1b[%dm", color);
}
#elif PL_WINDOWS
#include <windows.h>
void set_print_color(int color)
{
	static HANDLE hConsole;
	if (!hConsole)
		hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
	SetConsoleTextAttribute(hConsole, color);
}
#endif

static FILE* log_file = NULL;
static size_t last_log_length = 0;

// Specifies the frame number the previous log was on, to indicate if a new message is on a new frame
static size_t last_log_frame = 0;
static int last_severity = 0;

int log_init()
{
	if (log_file)
		return 1;

	last_log_length = 0;
	last_log_frame = 0;

	char fname[256];
	time_t rawtime;
	struct tm* timeinfo;

	time(&rawtime);
	timeinfo = localtime(&rawtime);
	create_dirs("./logs");
	strftime(fname, sizeof fname, "./logs/%F_%H", timeinfo);
	strcat(fname, ".log");
	log_file = fopen(fname, "w");
	if (log_file == NULL)
		return -1;
	return 0;
}

void log_terminate()
{
	if (log_file == NULL)
		return;
	fclose(log_file);
	log_file = NULL;
}

#ifdef DEBUG
#define WRITE(s)        \
	fputs(s, stdout);   \
	fputs(s, log_file); \
	fflush(log_file);
#else
#define WRITE(s)      \
	fputs(s, stdout); \
	fputs(s, log_file);
#endif

static int message_count[LOG_SEVERIY_MAX] = {0};

int log_get_count(int severity)
{
	if ((unsigned int)severity >= LOG_SEVERIY_MAX)
		return -1;

	return message_count[severity];
}

int log_call(int severity, const char* name, const char* fmt, ...)
{
	if (severity <= -1)
		severity = last_severity;

	if (severity >= LOG_SEVERIY_MAX)
		severity = 0;

	++message_count[severity];

	static const int color_map[] = {CONSOLE_WHITE, CONSOLE_BLUE, CONSOLE_YELLOW, CONSOLE_RED, CONSOLE_MAGENTA};
	set_print_color(color_map[severity]);
	last_severity = severity;

	char buf[2048];

	// Divider between frames
	if (last_log_frame != time_framecount())
	{
		last_log_frame = time_framecount();
		// Write a divider with the length of last_log_length capped at 64 characters
		last_log_length = min(64, last_log_length);
		memset(buf, '-', last_log_length);
		buf[last_log_length] = '\n';
		buf[last_log_length + 1] = '\0';
	}

	last_log_length = 0;

	// Header
	if (name)
	{
		strcpy(buf, "[ ");
		strcat(buf, name);
		strcat(buf, " @ ");

		time_t rawtime;
		struct tm* timeinfo;

		time(&rawtime);
		timeinfo = localtime(&rawtime);

		strftime(buf + strlen(buf), sizeof buf, "%H:%M.%S", timeinfo);
		strcat(buf, " ]");

		if (severity == LOG_SEVERITY_ASSERT)
		{
			strcat(buf, " ASSERT ");
		}
		else if (severity == LOG_SEVERITY_ERROR)
		{
			strcat(buf, " ERROR ");
		}
		else if (severity == LOG_SEVERITY_WARNING)
		{
			strcat(buf, " WARNING ");
		}

		strcat(buf, ": ");
		WRITE(buf);
	}
	else
	{
		WRITE(" -> ");
	}

	// Format the message
	va_list args;
	va_start(args, fmt);
	string_vformat(buf, sizeof buf, fmt, args);
	va_end(args);

	WRITE(buf);
	WRITE("\n");

	set_print_color(CONSOLE_WHITE);
	return last_log_length;
}