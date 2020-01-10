#include "log.h"
#include "utils.h"
#include "math/math_extra.h"
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

static void ftoa_fixed(char* buffer, double value);
static void ftoa_sci(char* buffer, double value);

#if PL_LINUX
void set_print_color(int color) { printf("", color); }
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

int normalize(double* val) {
	int exponent = 0;
	double value = *val;

	while (value >= 1.0) {
		value /= 10.0;
		++exponent;
	}

	while (value < 0.1) {
		value *= 10.0;
		--exponent;
	}
	*val = value;
	return exponent;
}

static void ftoa_fixed(char* buffer, double value) {
	/* carry out a fixed conversion of a double value to a string, with a precision of 5 decimal digits.
	 * Values with absolute values less than 0.000001 are rounded to 0.0
	 * Note: this blindly assumes that the buffer will be large enough to hold the largest possible result.
	 * The largest value we expect is an IEEE 754 double precision real, with maximum magnitude of approximately
	 * e+308. The C standard requires an implementation to allow a single conversion to produce up to 512
	 * characters, so that's what we really expect as the buffer size.
	 */

	int exponent = 0;
	int places = 0;
	static const int width = 4;

	if (value == 0.0) {
		buffer[0] = '0';
		buffer[1] = '\0';
		return;
	}

	if (value < 0.0) {
		*buffer++ = '-';
		value = -value;
	}

	exponent = normalize(&value);

	while (exponent > 0) {
		int digit = value * 10;
		*buffer++ = digit + '0';
		value = value * 10 - digit;
		++places;
		--exponent;
	}

	if (places == 0)
		*buffer++ = '0';

	*buffer++ = '.';

	while (exponent < 0 && places < width) {
		*buffer++ = '0';
		--exponent;
		++places;
	}

	while (places < width) {
		int digit = value * 10.0;
		*buffer++ = digit + '0';
		value = value * 10.0 - digit;
		++places;
	}
	*buffer = '\0';
}

// Converts a float to scientific notation
void ftoa_sci(char* buffer, double value) {
	int exponent = 0;
	int places = 0;
	static const int width = 4;

	// If value is 0, 
	if (value == 0.0) {
		buffer[0] = '0';
		buffer[1] = '\0';
		return;
	}

	if (value < 0.0) {
		*buffer++ = '-';
		value = -value;
	}

	exponent = normalize(&value);

	int digit = value * 10.0;
	*buffer++ = digit + '0';
	value = value * 10.0 - digit;
	--exponent;

	*buffer++ = '.';

	for (int i = 0; i < width; i++) {
		int digit = value * 10.0;
		*buffer++ = digit + '0';
		value = value * 10.0 - digit;
	}

	*buffer++ = 'e';
	itoa(exponent, buffer, 10);
}


FILE* log_file = NULL;
#if DEBUG
#define WRITE(s) fputs(s, stdout), fputs(s, log_file); fflush(log_file);
#else
#define WRITE(s) fputs(s, stdout), fputs(s, log_file);
#endif
#define FLUSH_BUFCH buffer[buffer_index] = '\0'; WRITE(buffer); buffer_index = 0;
#define WRITECH(c)  buffer[buffer_index] = c; buffer_index++ ; if(buffer_index == 510){ FLUSH_BUFCH } ;

void log_init()
{
	if (log_file)
		return;

	char fname[256];
	time_t rawtime;
	struct tm* timeinfo;

	time(&rawtime);
	timeinfo = localtime(&rawtime);
	create_dirs("./logs");
	strftime(fname, sizeof fname, "./logs/%F_%H.%M", timeinfo);
	strcat(fname, ".log");
	log_file = fopen(fname, "w");
}

void log_terminate()
{
	if (log_file == NULL)
		return;
	fclose(log_file);
	log_file = NULL;
}

int log_call(int color, const char* name, const char* fmt, ...) 
{
	va_list arg;

	va_start(arg, fmt);

	int int_tmp;
	char char_tmp;
	char* string_tmp;
	double double_tmp;
	set_print_color(color);
	// When chars in format are written they are saved to buf
	// After 512 chars or a flag is hit, buffer is flushed
	size_t buffer_index = 0;
	// Write the header
	char buffer[512];
	{
		strcpy(buffer, "[ ");
		strcat(buffer, name);
		strcat(buffer, " @ ");

		time_t rawtime;
		struct tm* timeinfo;

		time(&rawtime);
		timeinfo = localtime(&rawtime);

		strftime(buffer + strlen(buffer), sizeof buffer, "%H.%M.%S", timeinfo);
		strcat(buffer, " ]: ");
		WRITE(buffer);
	}

	char ch;
	int length = 0;
	int is_long = 0;
	while (ch = *fmt++) 
	{
		if ('%' == ch)
		{
			FLUSH_BUFCH;
			switch (ch = *fmt++) 
			{
				// %% precent sign
			case '%':
				WRITECH('%');
				length++;
				break;

				// %c character
			case 'c':
				char_tmp = va_arg(arg, int);
				WRITECH(char_tmp);
				length++;
				break;

				// %s string
			case 's':
				string_tmp = va_arg(arg, char*);
				WRITE(string_tmp);
				length += strlen(string_tmp);
				break;

				// %d int
			case 'd':
				int_tmp = va_arg(arg, int);
				itoa(int_tmp, buffer, 10);
				WRITE(buffer);
				length += strlen(buffer);
				break;

				// %x int hex
			case 'x':
				int_tmp = va_arg(arg, int);
				itoa(int_tmp, buffer, 16);
				WRITE(buffer);
				length += strlen(buffer);
				break;

			// %f float
			case 'f':
				double_tmp = va_arg(arg, double);
				ftoa_fixed(buffer, double_tmp);
				WRITE(buffer);
				length += strlen(buffer);
				break;
			// %e float in scientific form
			case 'e':
				double_tmp = va_arg(arg, double);
				ftoa_sci(buffer, double_tmp);
				WRITE(buffer);
				length += strlen(buffer);
				break;
			}
		}
		else {
			WRITECH(ch);
			length++;
		}
	}
	WRITECH('\n');
	FLUSH_BUFCH;
	set_print_color(CONSOLE_WHITE);
	va_end(arg);
	return length;
}

#define BUF_WRITE(s) if(buf){ strcpy(buf, s); buf+=strlen(s); } length+=strlen(s);
int vformat(char* buf, size_t size, char const* fmt, va_list arg) {

	int int_tmp;
	char char_tmp;
	char* string_tmp;
	double double_tmp;

	char buf_tmp[512];

	char ch;
	int length = 0;
	int is_long = 0;
	while (ch = *fmt++) {
		is_long = 0;
		if (ch == '%') {
			switch (ch = *fmt++) {
				/* %% - print out a single %    */
			case '%':
				if (buf)
				{
					*buf = '%';
					buf++;
				}
				length++;
				break;

				/* %c: print out a character    */
			case 'c':
				if (buf)
				{
					*buf = va_arg(arg, int);
					buf++;
				}
				length++;
				break;

				/* %s: print out a string       */
			case 's':
				string_tmp = va_arg(arg, char*);
				BUF_WRITE(string_tmp)

					break;

				/* %d: print out an int         */
			case 'd':
				int_tmp = va_arg(arg, int);
				itoa(int_tmp, buf_tmp, 10);
				BUF_WRITE(buf_tmp)
					break;

				/* %x: print out an int in hex  */
			case 'x':
				int_tmp = va_arg(arg, int);
				itoa(int_tmp, buf_tmp, 16);
				BUF_WRITE(buf_tmp)

					break;

			case 'f':
				double_tmp = va_arg(arg, double);
				ftoa_fixed(buf_tmp, double_tmp);
				BUF_WRITE(buf_tmp)

					break;

			case 'e':
				double_tmp = va_arg(arg, double);
				ftoa_sci(buf_tmp, double_tmp);
				BUF_WRITE(buf_tmp)

					break;
			}
		}
		else if (ch == 'l')
		{
			is_long = 1;
		}
		else if (ch == 'z')
		{
			is_long = 2;
		}
		else
		{
			*buf = ch;
			buf++;
			length++;
		}
		if (buf)
			*buf = '\0';
	}
	return length;
}