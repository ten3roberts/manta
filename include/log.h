#ifndef LOG_H
#define LOG_H

#include <string.h>

#if PL_LINUX
#define CONSOLE_BLACK	0
#define CONSOLE_RED		31
#define CONSOLE_GREEN	32
#define CONSOLE_YELLOW	33
#define CONSOLE_BLUE	34
#define CONSOLE_MAGENTA 35
#define CONSOLE_CYAN	36
#define CONSOLE_WHITE	37
#define __FILENAME__	(strrchr(__FILE__, '/') ? strrchr(__FILE__, '/') + 1 : __FILE__)
#elif PL_WINDOWS
#define CONSOLE_BLACK	0
#define CONSOLE_RED		12
#define CONSOLE_GREEN	2
#define CONSOLE_YELLOW	6
#define CONSOLE_BLUE	1
#define CONSOLE_MAGENTA 5
#define CONSOLE_CYAN	9
#define CONSOLE_WHITE	15
#define __FILENAME__	(strrchr(__FILE__, '\\') ? strrchr(__FILE__, '\\') + 1 : __FILE__)
#endif

// Needs to be called before any other log function
// Creates the log file
// Multiple calls are safe as they are ignored
int log_init();

// Should be called at the end of the program
// Flushes and closes the log file correctly
// Multiple calls are safe as they are ignored
void log_terminate();

// Issues a formated log call that prints to stdout and a log file
// name will be printed inside the header, "[ name @ %H:%M.%S ] {NONE, CONSOLE_RED:'WARNING', CONSOLE_YELLOW:'ERROR'}: "
// If name is NULL, the header will not be printed
// Can be called either directly or via the LOG_* macros provided for autmatic name and color assignment
// Severity indicates the different levels
// -> 0 : normal message
// -> 1 : status message
// -> 2 : warning message
// -> 3 : error message
// -> 4 : assertion
// Errors will cause a short delay in debug mode but not in release mode. This is too slow down so that the first error doesn't go beyond term scrollback
// Severity 4 (assert) will abort the program
// Severity cannot any other than these listed

#define LOG_SEVERITY_NORMAL	 0
#define LOG_SEVERITY_STATUS	 1
#define LOG_SEVERITY_WARNING 2
#define LOG_SEVERITY_ERROR	 3
#define LOG_SEVERITY_ASSERT	 4
#define LOG_SEVERIY_MAX		 LOG_SEVERITY_ASSERT

// Returns the count of messages printed with specified severity
int log_get_count(int severity);

int log_call(int severity, const char* name, const char* fmt, ...);

// Continues the previous log call
#define LOG_CONT(fmt, ...) log_call(-1, NULL, fmt, ##__VA_ARGS__)

// Issues a formated log call that prints to stdout and a log file
// Prints in white color and indicates a normal message
#define LOG(fmt, ...) log_call(0, __FILENAME__, fmt, ##__VA_ARGS__)

// Issues a formated log call that prints to stdout and a log file
// Prints in blue color and indicates a status message
#define LOG_S(fmt, ...) log_call(1, __FILENAME__, fmt, ##__VA_ARGS__)

// Issues a formated log call that prints to stdout and a log file
// Prints in yellow color and indicates a warning or non-significant message
#define LOG_W(fmt, ...) log_call(2, __FILENAME__, fmt, ##__VA_ARGS__)

// Issues a formated log call that prints to stdout and a log file
// Prints in red color and indicates an error message
#define LOG_E(fmt, ...) log_call(3, __FILENAME__, fmt, ##__VA_ARGS__)

// Asserts the program with a message if cond equals to zero
// Aborts all execution after 1 second to allow the user to read before terminal closes
#define LOG_ASSERT(cond, fmt, ...) \
	if ((cond) == 0)               \
	log_call(LOG_SEVERITY_ASSERT, __FILENAME__, fmt, ##__VA_ARGS__)

#endif