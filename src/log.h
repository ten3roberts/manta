#include <string.h>

#if PL_LINUX
#define CONSOLE_WHITE 37
#define CONSOLE_GREEN 32
#define CONSOLE_YELLOW 33
#define CONSOLE_RED 31
#define __FILENAME__ (strrchr(__FILE__, '/') ? strrchr(__FILE__, '/') + 1 : __FILE__)
#elif PL_WINDOWS
#define CONSOLE_WHITE 15
#define CONSOLE_GREEN 2
#define CONSOLE_YELLOW 6
#define CONSOLE_RED 12
#define __FILENAME__  (strrchr(__FILE__, '\\') ? strrchr(__FILE__, '\\') + 1 : __FILE__)
#endif

// Needs to be called before any other log function
// Creates the log file
// Multiple calls are safe as they are ignored
void log_init();

// Should be called at the end of the program
// Flushes and closes the log file correctly
// Multiple calls are safe as they are ignored
void log_terminate();

// Issues a formated log call that prints to stdout and a log file
int log_call(int color, const char* name, const char * fmt, ...);

 
// Issues a formated log call that prints to stdout and a log file
// Prints in white color and indicates a normal message
#define LOG(fmt, ...) log_call(CONSOLE_WHITE, __FILENAME__, fmt, __VA_ARGS__)

// Issues a formated log call that prints to stdout and a log file
// Prints in green color and indicates an ok or successful status message
#define LOG_OK(fmt, ...) log_call(CONSOLE_GREEN, __FILENAME__, fmt, __VA_ARGS__)

// Issues a formated log call that prints to stdout and a log file
// Prints in yellow color and indicates a warning or non-significant message
#define LOG_W(fmt, ...) log_call(CONSOLE_YELLOW, __FILENAME__, fmt, __VA_ARGS__)

// Issues a formated log call that prints to stdout and a log file
// Prints in red color and indicates and error message
#define LOG_E(fmt, ...) log_call(CONSOLE_RED, __FILENAME__, fmt, __VA_ARGS__)